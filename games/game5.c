/*
 * game4 : Knight's Tour (기사의 여행)
 *
 * 체스마의 행마법을 이용해 중복 없이 지정된 크기(5x5, 6x6, 7x7)의 판을 모두 채우는 게임.
 * - 난이도가 높을수록 최고 보너스 점수 상향 및 초당 점수 감점량 감소 규칙 적용
 *
 * 사용 라이브러리 : C 표준 라이브러리 및 POSIX 타임 인터페이스
 * 실행 인자       : argv[1] = 로비에서 넘어온 사용자 ID
 * 종료 코드       : 최종 계산된 점수 (0~255 clamp). 로비 WEXITSTATUS 회수
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

// 팀원들의 스타일로 이스케이프 시퀀스 밀림 방지(EL)를 적용한 그리기 기능
static void draw_board(int elapsed_time) {
    cursor_home();
    printf("=========================================" EL "\n");
    printf("   기사의 여행 (Knight's Tour) - game4"    EL "\n");
    printf("   [ 실시간 경과 시간: %d초 ]"             EL "\n", elapsed_time);
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
    printf("* 조작법: w/a/s/d (이동), 공백+Enter (착수), q (포기)" EL "\n");
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

// 리눅스 표준 표준 입력 파싱 처리 함수 
// 리눅스 콘솔 엔터 없는 실시간 입력 및 방향키 파싱 함수
static int read_action(char *out) {
    struct termios oldt, newt;
    int ch;
    
    // 1. 현재 터미널 설정 저장
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    
    // 2. 입력 버퍼링(ICANON) 및 에코(ECHO) 비활성화
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    // 3. 한 글자 읽기
    ch = getchar();
    
    // 4. 만약 방향키 이스케이프 시퀀스(\033[...)가 들어온 경우 처리
    if (ch == 27) { // 27은 ESC 문자 코드
        ch = getchar();
        if (ch == '[') {
            ch = getchar();
            switch (ch) {
                case 'A': *out = 'w'; break; // 위쪽 화살표 -> w로 매핑
                case 'B': *out = 's'; break; // 아래쪽 화살표 -> s로 매핑
                case 'C': *out = 'd'; break; // 오른쪽 화살표 -> d로 매핑
                case 'D': *out = 'a'; break; // 왼쪽 화살표 -> a로 매핑
                default:  *out = ' '; break;
            }
            // 터미널 설정을 원상복구하고 리턴
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return 1;
        }
    }
    
    // 5. 방향키가 아닌 일반 키(q, 스페이스바 등) 처리
    *out = (char)ch;
    
    // 6. 터미널 설정을 기존 상태로 안전하게 원상복구
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 1;
}

// 게임 결과 요약 정산창 출력 함수
static void show_summary(int status, int elapsed_time, int score) {
    printf("\033[2J\033[H"); // 화면 대청소
    printf("=========================================\n");
    printf("        기사의 여행 (Game 4) 결산\n");
    printf("=========================================\n");
    printf(" Player       : %s\n", username);
    printf(" 맵 난이도    : %d x %d\n", N, N);
    printf(" 총 소요 시간 : %d초\n", elapsed_time);
    printf("-----------------------------------------\n");
    
    if (status == 1) {
        printf(" [결과] 완벽하게 보드를 정복하여 승리했습니다! \n");
    } else if (status == -1) {
        printf(" [결과] 막다른 길에 갇혀 실패했습니다. (패배) \n");
    } else {
        printf(" [결과] 중도 포기하셨습니다. \n");
    }
    
    printf("=========================================\n");
    printf(" 최종 획득 점수: %d 점\n", score);
    printf("=========================================\n");
    printf("\n아무 키나 누르고 Enter를 치면 로비로 돌아갑니다...");
    fflush(stdout);
    char dummy;
    if (scanf(" %c", &dummy)) { /* 대기 */ }
}

int main(int argc, char **argv) {
    // 로비 규약 연동: 유저 이름 받기
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

    // 2. 메인 리눅스 표준 게임 루프
    while (1) {
        elapsed_time = (int)difftime(time(NULL), start_time);
        draw_board(elapsed_time);

        status = check_game_over();
        if (status != 0) break; // 승리 혹은 패배 시 루프 종료

        char act = ' ';
        if (!read_action(&act)) break; // EOF 발생 시 종료

        if (act == 'q' || act == 'Q') {
            status = 0; // 중도 포기
            break;
        }

        // 턴제 기반 표준 w,a,s,d 및 착수 명령어 매핑
        switch (act) {
            case 'w': case 'W': if (cursor_y > 0) cursor_y--; break;
            case 's': case 'S': if (cursor_y < N - 1) cursor_y++; break;
            case 'a': case 'A': if (cursor_x > 0) cursor_x--; break;
            case 'd': case 'D': if (cursor_x < N - 1) cursor_x++; break;
            
            case ' ': // 공백 문자 진입 시 착수 수행
                if (is_valid_move(knight_x, knight_y, cursor_x, cursor_y)) {
                    move_count++;
                    board[cursor_y][cursor_x] = move_count;
                    knight_x = cursor_x;
                    knight_y = cursor_y;
                } else {
                    // 리눅스 콘솔 비프 사운드 이스케이프 시퀀스
                    printf("\a");
                    fflush(stdout);
                }
                break;
        }
    }

    // 3. 점수 계산 페이즈 (난이도 가중치 반영)
    int score = 0;
    if (status == 1) {
        int base_score = 10000;
        int penalty_per_second = 50;

        if (N == 5) {
            base_score = 10000;
            penalty_per_second = 60;
        } else if (N == 6) {
            base_score = 15000;
            penalty_per_second = 40;
        } else if (N == 7) {
            base_score = 20000;
            penalty_per_second = 20;
        }

        score = base_score - (elapsed_time * penalty_per_second);
        if (score < 1000) score = 1000;
    } else {
        score = 0; // 실패 혹은 기권은 0점 처리
    }

    // 0~255 내부로 락을 걸어 시스템 콜 범위 클램핑 
    int exit_score = score;
    if (exit_score > 255) exit_score = 255;
    if (exit_score < 0) exit_score = 0;

   // =================================================================
    // game4.c 파일 main 함수 맨 아래 (return exit_score; 바로 전) 수정
    // =================================================================
    show_summary(status, elapsed_time, score);

    // [추가] 팀원 파이프 규약 연동: argv[3]으로 전달받은 파이프 번호에 점수 기록하기
    if (argc >= 4 && argv[3]) {
        int pipe_fd = atoi(argv[3]);
        if (pipe_fd > 0) {
            // 부모 프로세스(로비)로 최종 점수 송신
            write(pipe_fd, &exit_score, sizeof(exit_score));
            close(pipe_fd);
        }
    }

    return exit_score;
}