/*
 * game5 : Knight's Tour (기사의 여행)
 *
 * 체스마의 행마법을 이용해 중복 없이 지정된 크기(5x5, 6x6, 7x7)의 판을 모두 채우는 게임.
 *
 * [업데이트 로그]
 * - 기본 점수: 클리어 여부와 상관없이 무조건 [차지한 칸 수 * 칸당 점수] 보장
 * - 추가 점수: 모든 칸을 다 채워 클리어 성공 시 [소요시간 비례 추가 보너스 점수] 가산
 * - 점수 제한 해제: 기존 상한선인 255점 제한 규칙을 삭제하여 300~400점대 고득점 반영 가능
 *
 * 사용 라이브러리 : C 표준 라이브러리 및 POSIX 타임 인터페이스
 * 실행 인자       : argv[1] = 로비에서 넘어온 사용자 ID
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>

/* 매 줄 끝 잔상 제거 및 화면 제어용 ANSI 이스케이프 시퀀스 */
#define EL "\033[K"

#define MAX_SIZE 7

// 게임 상태 전역 변수
static int board[MAX_SIZE][MAX_SIZE];
static int N = 5; 
static int knight_x = -1, knight_y = -1; 
static int cursor_x = 0, cursor_y = 0;   
static int move_count = 0;
static char username[64];

// 나이트 이동 8방향 
static const int dx[8] = { -2, -1, 1, 2, 2, 1, -1, -2 };
static const int dy[8] = { 1, 2, 2, 1, -1, -2, -2, -1 };

// 화면 초기화 함수 
static void screen_init(void) {
    printf("\033[2J\033[3J\033[H");
    fflush(stdout);
}

// 커서 좌상단 복귀 (화면 깜빡임 방지용 고속 덮어쓰기)
static void cursor_home(void) {
    printf("\033[H");
}

static void init_board(void) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            board[i][j] = 0;
        }
    }
    knight_x = -1;
    knight_y = -1;
    cursor_x = 0;
    cursor_y = 0;
    move_count = 0;
}

static int is_valid_move(int start_x, int start_y, int end_x, int end_y) {
    if (start_x == -1 && start_y == -1) return 1;
    if (end_x < 0 || end_x >= N || end_y < 0 || end_y >= N) return 0;
    if (board[end_y][end_x] > 0) return 0;

    for (int i = 0; i < 8; i++) {
        if (start_x + dx[i] == end_x && start_y + dy[i] == end_y) {
            return 1;
        }
    }
    return 0;
}

// 이스케이프 시퀀스 밀림 방지(EL)를 적용한 그리기 기능
static void draw_board(int elapsed_time) {
    cursor_home();
    printf("=========================================" EL "\n");
    printf("   기사의 여행 (Knight's Tour) - Game 5"     EL "\n");
    printf("   [ 실시간 경과 시간: %d초 ]"              EL "\n", elapsed_time);
    printf("=========================================" EL "\n");
    printf(" Player : %s (크기: %dx%d)"               EL "\n", username, N, N);
    printf("-----------------------------------------" EL "\n");

    // 상단 테두리
    printf("   " EL);
    for (int i = 0; i < N; i++) printf("+---");
    printf("+\n");

    for (int i = 0; i < N; i++) {
        printf(" %d " EL, i + 1); // 행 번호
        for (int j = 0; j < N; j++) {
            printf("|");
            if (j == cursor_x && i == cursor_y) {
                if (j == knight_x && i == knight_y) printf("[K]");
                else if (board[i][j] > 0) printf("[%2d]", board[i][j]);
                else printf("[ ]");
            } else {
                if (j == knight_x && i == knight_y) printf(" K "); 
                else if (board[i][j] > 0) printf("%3d", board[i][j]); 
                else printf("   "); 
            }
        }
        printf("|\n");

        // 중간 테두리
        printf("   " EL);
        for (int j = 0; j < N; j++) printf("+---");
        printf("+\n");
    }

    printf("\n* 이동 횟수: %d / %d" EL "\n", move_count, N * N);
    printf("* 조작법: w/a/s/d 또는 방향키 (이동), 스페이스바 (착수), q (포기)" EL "\n");
    if (knight_x != -1) {
        printf("* 현재 나이트 위치: (%d, %d)" EL "\n", knight_x + 1, knight_y + 1);
    } else {
        printf("* 현재 나이트 위치: 시작 전 (첫 자리를 고르고 착수하세요)" EL "\n");
    }
    printf("-----------------------------------------" EL "\n");
    printf("\033[J"); // 아래 잔상 청소
    printf(" 선택 > ");
    fflush(stdout);
}

static int check_game_over(void) {
    if (move_count == N * N) return 1; // 성공
    if (knight_x == -1) return 0;      // 시작 전

    for (int i = 0; i < 8; i++) {
        int nx = knight_x + dx[i];
        int ny = knight_y + dy[i];
        if (nx >= 0 && nx < N && ny >= 0 && ny < N && board[ny][nx] == 0) {
            return 0; // 갈 곳 남음
        }
    }
    return -1; // 실패 (막힘)
}

// 리눅스 콘솔 엔터 없는 실시간 입력 및 방향키 파싱 함수
static int read_action(char *out) {
    struct termios oldt, newt;
    int ch;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    ch = getchar();
    
    if (ch == 27) { // ESC 문자 코드
        ch = getchar();
        if (ch == '[') {
            ch = getchar();
            switch (ch) {
                case 'A': *out = 'w'; break;
                case 'B': *out = 's'; break;
                case 'C': *out = 'd'; break;
                case 'D': *out = 'a'; break;
                default:  *out = ' '; break;
            }
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return 1;
        }
    }
    
    *out = (char)ch;
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 1;
}

int main(int argc, char **argv) {
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        snprintf(username, sizeof(username), "%s", argv[1]);
    } else {
        snprintf(username, sizeof(username), "guest");
    }

    // 1. 난이도 선택 페이즈
    screen_init();
    printf("=========================================\n");
    printf("      인하대 PBL - 기사의 여행\n");
    printf("=========================================\n\n");
    printf("게임판의 크기를 선택하세요 (5, 6, 7): ");
    fflush(stdout);
    
    while (1) {
        char input_buf[16];
        if (!fgets(input_buf, sizeof(input_buf), stdin)) continue;
        if (sscanf(input_buf, "%d", &N) == 1 && N >= 5 && N <= 7) break;
        printf("5, 6, 7 중에서만 입력해주세요: ");
        fflush(stdout);
    }
    
    init_board();
    screen_init();

    time_t start_time = time(NULL);
    int status = 0;
    int elapsed_time = 0;

    // 2. 메인 게임 루프
    while (1) {
        elapsed_time = (int)difftime(time(NULL), start_time);
        draw_board(elapsed_time);

        status = check_game_over();
        if (status != 0) break;

        char act = ' ';
        if (!read_action(&act)) break;

        if (act == 'q' || act == 'Q') {
            status = 0; // 포기
            break;
        }

        switch (act) {
            case 'w': case 'W': if (cursor_y > 0) cursor_y--; break;
            case 's': case 'S': if (cursor_y < N - 1) cursor_y++; break;
            case 'a': case 'A': if (cursor_x > 0) cursor_x--; break;
            case 'd': case 'D': if (cursor_x < N - 1) cursor_x++; break;
            
            case ' ': 
                if (is_valid_move(knight_x, knight_y, cursor_x, cursor_y)) {
                    move_count++;
                    board[cursor_y][cursor_x] = move_count;
                    knight_x = cursor_x;
                    knight_y = cursor_y;
                } else {
                    printf("\a");
                    fflush(stdout);
                }
                break;
        }
    }

    // 3. 점수 계산 페이즈 
    int score = 0;
    int points_per_cell = 0;   
    int max_time_bonus = 0;    
    int penalty_per_second = 0;

    if (N == 5) {
        points_per_cell = 4;    // 25칸 * 4점 = 다 채우면 기본 최대 100점
        max_time_bonus = 100;   
        penalty_per_second = 2; 
    } else if (N == 6) {
        points_per_cell = 5;    // 36칸 * 5점 = 다 채우면 기본 최대 180점
        max_time_bonus = 150;   
        penalty_per_second = 1; 
    } else if (N == 7) {
        points_per_cell = 6;    // 49칸 * 6점 = 다 채우면 기본 최대 294점
        max_time_bonus = 300;   
        penalty_per_second = 1; 
    }

    // 1) 기본 점수: 밟은 누적 칸 수 * 칸당 점수
    score = move_count * points_per_cell;

    // 2) 추가 보너스: 올 클리어 시 시간 보너스 합산
    if (status == 1) {
        int time_bonus = max_time_bonus - (elapsed_time * penalty_per_second);
        if (time_bonus < 20) time_bonus = 20; // 최소 보너스 보장
        
        score += time_bonus;
        printf("\n[Clear] ★ 축하합니다! 모든 칸을 성공적으로 완공하여 클리어 타임 보너스 %d점이 추가되었습니다! ★\n", time_bonus);
    } else {
        printf("\n[Game Over] 진행 도중 막혔거나 포기했습니다. (최종 획득 칸수: %d칸)\n", move_count);
    }

    // 최소값 안전장치만 유지 (점수가 음수로 떨어지는 것 방지)
    if (score < 0) score = 0;

    int exit_score = score;

    // [안전 버퍼 동기화 패치] 
    tcflush(STDIN_FILENO, TCIFLUSH);

    // argv[3]으로 전달받은 파이프 번호에 점수 기록하기
    if (argc >= 4 && argv[3]) {
        int pipe_fd = atoi(argv[3]);
        if (pipe_fd > 0) {
            write(pipe_fd, &exit_score, sizeof(exit_score));
            close(pipe_fd);
        }
    }

    return exit_score;
}