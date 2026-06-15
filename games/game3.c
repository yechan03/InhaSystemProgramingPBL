#define _XOPEN_SOURCE 600 /* select(), usleep() 노출을 위한 POSIX/XOpen 표준 지정 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

/* ──────────────── ANSI 제어 매크로 ──────────────── */
#define ESC "\033"
#define RST ESC "[0m"
#define BOLD ESC "[1m"
#define EL "\033[K" /* 줄 끝까지 지우기 잔상 방지 */

// 테트리스 가이드라인 공식 지정 미노 색상 매크로
#define C_I ESC "[96m" // Cyan
#define C_O ESC "[93m" // Yellow
#define C_T ESC "[35m" // Magenta
#define C_S ESC "[92m" // Green
#define C_Z ESC "[31m" // Red
#define C_L "\033[38;5;214m" // Orange
#define C_J "\033[34m"       // Blue
#define C_GR ESC "[90m" // Gray (벽/고스트)

#define BOARD_ROWS 20
#define BOARD_COLS 10

typedef enum {
    MINO_EMPTY = 0, MINO_I, MINO_O, MINO_T, MINO_S, MINO_Z, MINO_L, MINO_J
} mino_t;

typedef struct {
    mino_t type;
    int matrix[4][4];
    int x, y;
    int rotation;
} piece_t;

/* ──────────────── 사운드 제어 매크로 ──────────────── */
static void trigger_sound(const char *type) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "sh scripts/play_sound_tetris.sh %s &", type);
    system(cmd);
}

/* ──────────────── 전역 상태 ──────────────── */
static int board[BOARD_ROWS][BOARD_COLS];
static piece_t current_piece;
static char player_name[64] = "player";
static int score = 0;
static int game_over = 0;

static const int mino_templates[8][4][4] = {
    [MINO_EMPTY] = {{0}},
    [MINO_I] = {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},
    [MINO_O] = {{0,1,1,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
    [MINO_T] = {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
    [MINO_S] = {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},
    [MINO_Z] = {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}},
    [MINO_L] = {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},
    [MINO_J] = {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}
};

/* ──────────────── 터미널 로우 모드 ──────────────── */
static struct termios orig_termios;
static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf(ESC "[?25h\n"); // 커서 보이기 및 개행
    fflush(stdout);
}
static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf(ESC "[?25l"); // 커서 숨기기
    fflush(stdout);
}

/* ──────────────── 실시간 키 입력 모니터링 ──────────────── */
static int kbhit_timeout(long usec) {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = usec;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

/* ================ 테트리스 로직 ================ */

/* ──────────────── 테트리스 전용 전역 변수 ──────────────── */
static mino_t next_queue[3]; // 다음에 등장할 블록 3개를 담는 대기열 (미노 미리보기 변수)
static int lock_delay_count = 0; // 한 블록이 바닥에서 회전한 횟수 카운터 (무한 회전 방지 변수)
static mino_t hold_queue = MINO_EMPTY; // 보관함에 들어있는 블록 종류 (기본값: 비어있음) (홀딩 변수)
static int can_hold = 1; // 현재 블록이 홀드 가능한 상태인지 여부 (홀드 연속 사용 방지 변수)
static int current_cpu_usage = 0;       // 현재 OS의 CPU 사용량 (화면 표시용)
static long base_drop_interval = 500000; // 기본 하강 속도 (0.5초 = 500,000 마이크로초)
static long drop_interval = 500000;      // 실제 게임 루프에서 사용하는 하강 속도 (CPU 사용량에 따라 가변적으로 조정됨)

static void spawn_piece_with_next(void);
static void update_system_speed(long *current_interval);

/* ──────────────── 7-Bag 랜덤 셔플 엔진 ──────────────── */
static mino_t bag[7];
static int bag_index = 7;

static void refill_bag(void) {
    int i;
    // 주머니에 7가지 블록을 차례대로 채움
    for (i = 0; i < 7; i++) {
        bag[i] = (mino_t)(i + 1);
    }
    // 피셔-예이츠 셔플 알고리즘으로 무작위로 섞음
    for (i = 6; i > 0; i--) {
        int j = rand() % (i + 1);
        mino_t temp = bag[i];
        bag[i] = bag[j];
        bag[j] = temp;
    }
    bag_index = 0;
}

static mino_t pull_next_piece(void) {
    if (bag_index >= 7) {
        refill_bag();
    }
    return bag[bag_index++];
}

/* ──────────────── 충돌 검사 함수 ──────────────── 
 * 충돌이 발생하면 1, 이동 가능한 안전한 영역이면 0을 반환
 */
static int check_collision(int next_x, int next_y, const int matrix[4][4]) {
    int r, c;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            // 4x4 행렬 중에서 실제 블록 조각(1)이 있는 칸만 검사
            if (matrix[r][c] != 0) {
                int target_x = next_x + c;
                int target_y = next_y + r;

                // 좌우 벽 경계 검사
                if (target_x < 0 || target_x >= BOARD_COLS) {
                    return 1; 
                }

                // 바닥 경계 검사 
                if (target_y >= BOARD_ROWS) {
                    return 1; 
                }

                // 이미 쌓여있는 기존 블록과의 충돌 검사
                if (target_y >= 0) {
                    if (board[target_y][target_x] != MINO_EMPTY) {
                        return 1; 
                    }
                }
            }
        }
    }
    return 0; // 모든 검사를 통과하면 '안전(0)' 반환
}


/* ──────────────── 퍼펙트 클리어 검사 함수 ──────────────── 
 * 보드 전체가 완벽하게 비어있으면 1(True), 블록이 한 칸이라도 남아있으면 0(False)을 반환합니다.
 */
static int is_perfect_clear(void) {
    int r, c;
    for (r = 0; r < BOARD_ROWS; r++) {
        for (c = 0; c < BOARD_COLS; c++) {
            if (board[r][c] != MINO_EMPTY) {
                return 0; // 블록 찌꺼기가 하나라도 있으면 즉시 실패 반환
            }
        }
    }
    return 1; // 맵 전체가 청정 구역이면 성공 반환
}

static void update_system_speed(long *current_interval);

/* ──────────────── 바닥 고정 및 라인 클리어 ──────────────── */
static void lock_and_clear_lines(void) {
    int r, c;

    // 현재 블록을 board 배열에 고정
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            if (current_piece.matrix[r][c] != 0) {
                int target_x = current_piece.x + c;
                int target_y = current_piece.y + r;
                
                if (target_y >= 0 && target_y < BOARD_ROWS && target_x >= 0 && target_x < BOARD_COLS) {
                    board[target_y][target_x] = current_piece.type;
                }
            }
        }
    }
    trigger_sound("lock");

    // 라인 클리어 검사 (아래에서부터 위로 탐색)
    int cleared_lines_count = 0;
    
    for (r = BOARD_ROWS - 1; r >= 0; r--) {
        int is_line_full = 1;
        for (c = 0; c < BOARD_COLS; c++) {
            if (board[r][c] == MINO_EMPTY) {
                is_line_full = 0; // 한 칸이라도 비어있으면 탈출
                break;
            }
        }

        // 한 줄이 가득 찼다면 지우고 위쪽 줄들을 아래로 한 칸씩 당김
        if (is_line_full) {
            cleared_lines_count++;
            
            int next_r;
            for (next_r = r; next_r > 0; next_r--) {
                for (c = 0; c < BOARD_COLS; c++) {
                    board[next_r][c] = board[next_r - 1][c];
                }
            }
            // 맨 윗줄은 빈칸으로 채움
            for (c = 0; c < BOARD_COLS; c++) {
                board[0][c] = MINO_EMPTY;
            }
            
            // 줄을 당겼으므로 현재 행(r)을 다시 한 번 검사해야 함
            r++; 
        }
    }

    // 점수 계산
    if (cleared_lines_count > 0) {
        // 기본 점수: 1줄당 10점
        int base_score = cleared_lines_count * 10;
        int bonus_score = 0;

        // 다중 라인 제거 추가 점수 규칙
        if (cleared_lines_count == 2)      bonus_score = 5;
        else if (cleared_lines_count == 3) bonus_score = 10;
        else if (cleared_lines_count == 4) bonus_score = 20;

        score += (base_score + bonus_score);

        /* ──────────────── 퍼펙트 클리어 검사 ──────────────── */
        // 줄을 성공적으로 지운 뒤, 화면에 블록이 단 하나도 남지 않았다면 올클리어 보너스 가산
        if (is_perfect_clear()) {
            score += 500; // 대형 업적 보너스 500점 추가 점수 가산!
            trigger_sound("PerfectClear");
        }
        else {
            trigger_sound("clear");
        }
    }

    // 게임 오버 체크 (스폰 지점인 0번 행 근처가 막혀있으면 게임 오버)
    for (c = 3; c <= 6; c++) {
        if (board[0][c] != MINO_EMPTY) {
            game_over = 1;
            return;
        }
    }

    // 다음 블록 스폰
    spawn_piece_with_next();

    // 루프에서 사용하는 타이머 주기 변수(예: drop_interval)의 주소를 업데이트
    update_system_speed(&drop_interval);
}

/* ──────────────── 하드 드롭 알고리즘 ──────────────── */
static void execute_hard_drop(void) {
    // 충돌이 발생하기 직전까지 y 좌표를 계속 아래로 밀어붙임
    while (!check_collision(current_piece.x, current_piece.y + 1, current_piece.matrix)) {
        current_piece.y++;
    }

    // 하드 드롭 성공 시 보너스 5점 가산
    score += 5;

    // 바닥에 완벽히 닿았으므로, 기존에 만든 바닥 고정 및 라인 클리어 함수를 즉시 호출
    lock_and_clear_lines();
}

/* ──────────────── 미노 회전 알고리즘 ──────────────── */

/* 블록을 시계(dir=1) 또는 반시계(dir=0) 방향으로 회전시키는 함수 */
static void rotate_matrix(int source[4][4], int dest[4][4], int dir) {
    // 미노 O는 회전하지 않음
    if (current_piece.type == MINO_O) {
        // 회전하지 않고 현재 모양 그대로 목적지에 복사만 해주고 탈출
        memcpy(dest, source, sizeof(int) * 4 * 4);
        return;
    }
    // 먼저 전체를 빈칸으로 초기화
    memset(dest, 0, sizeof(int) * 4 * 4);
    
    // Case 1: 조작 중인 블록이 T를 제외한 블록인 경우
    if (current_piece.type == MINO_I || current_piece.type == MINO_L || 
        current_piece.type == MINO_J || current_piece.type == MINO_S || 
        current_piece.type == MINO_Z) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (dir == 1) { // 시계 방향 회전
                    dest[c][3 - r] = source[r][c];
                } else {        // 반시계 방향 회전
                    dest[3 - c][r] = source[r][c];
                }
            }
        }
    } 
    // Case 2: T 미노인 경우
    else {
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                if (dir == 1) { // 시계 방향 회전
                    dest[c][2 - r] = source[r][c];
                } else {        // 반시계 방향 회전
                    dest[2 - c][r] = source[r][c];
                }
            }
        }
    }
}

// [현재 회전상태 -> 다음 회전상태][5가지 테스트 단계][X, Y 오프셋]
static const int wall_kick_data[4][5][2] = {
    // 0 (북) -> 1 (동) 
    [0] = { {0, 0}, {-1, 0}, {-1,  1}, {0, -2}, {-1, -2} },
    // 1 (동) -> 2 (남)
    [1] = { {0, 0}, { 1, 0}, { 1, -1}, {0,  2}, { 1,  2} },
    // 2 (남) -> 3 (서)
    [2] = { {0, 0}, { 1, 0}, { 1,  1}, {0, -2}, { 1, -2} },
    // 3 (서) -> 0 (북)
    [3] = { {0, 0}, {-1, 0}, {-1, -1}, {0,  2}, {-1,  2} }
};



/* ──────────────── SRS 월 킥 회전 알고리즘 ──────────────── */
static void execute_srs_rotation(int dir) {
    int next_matrix[4][4] = {{0}};
    // 바닥에 붙은 상태에서 이미 15번 넘게 돌렸다면 더 이상 회전 시도 자체를 막아서 무한 회전 방지 (lock_delay_count는 블록이 바닥에 닿은 이후 회전 시도마다 증가, 새 블록이 스폰되면 초기화)
    if (check_collision(current_piece.x, current_piece.y + 1, current_piece.matrix)) {
        if (lock_delay_count >= 15) {
            lock_and_clear_lines(); // 즉시 바닥에 굳혀버림
            return;
        }
    }
    // 조작 중인 블록을 가상으로 회전
    rotate_matrix(current_piece.matrix, next_matrix, dir);

    // 회전 후 변할 미래의 상태 계산 (0~3 범위 순환)
    int current_rot = current_piece.rotation;
    int next_rot = current_rot;
    if (dir == 1) next_rot = (current_rot + 1) % 4;
    else          next_rot = (current_rot + 3) % 4;

    // 월 킥 데이터셋 중에서 어떤 오프셋 라인을 참조할지 인덱스 바인딩
    // 반시계 방향 회전일 경우, 데이터 구조 대칭성을 위해 기준 축을 반전하여 참조
    int kick_index = (dir == 1) ? current_rot : next_rot;

    int test;
    // 5가지 테스트 단계를 순서대로 검사
    for (test = 0; test < 5; test++) {
        int kick_x = wall_kick_data[kick_index][test][0];
        int kick_y = wall_kick_data[kick_index][test][1];

        // 반시계 회전일 경우 가이드라인 법칙에 따라 오프셋 부호를 반대로 뒤집어 연산
        if (dir == 0) {
            kick_x = -kick_x;
            kick_y = -kick_y;
        }

        int target_x = current_piece.x + kick_x;
        int target_y = current_piece.y - kick_y;

        // 이 오프셋만큼 밀어냈을 때 충돌이 없는 안전한 곳인지 검사
        if (!check_collision(target_x, target_y, next_matrix)) {
            /* ──────────────── [ 고도화: 스핀 판정 엔진 ] ──────────────── */
            // 조건 1: 제자리 회전(test == 0)이 아니라 킥 변위(test > 0)를 받아 회전 성공함
            // 조건 2: 회전 직후 상/하/좌/우 중 3곳 이상이 벽이나 고정 블록으로 꽉 막혀있음
            if (test > 0) {
                int blocked_count = 0;
                if (check_collision(target_x + 1, target_y, next_matrix)) blocked_count++; // 우측 막힘
                if (check_collision(target_x - 1, target_y, next_matrix)) blocked_count++; // 좌측 막힘
                if (check_collision(target_x, target_y + 1, next_matrix)) blocked_count++; // 바닥 막힘
                if (check_collision(target_x, target_y - 1, next_matrix)) blocked_count++; // 천장 막힘

                // 3면 이상 꽉 끼인 상태라면 미노 타입별 고유 스핀 이펙트 가동!
                if (blocked_count >= 3) {
                    printf("\033[1;33m\n[SPIN] ");
                    switch (current_piece.type) {
                        case MINO_L: printf("L-SPIN!"); break;
                        case MINO_J: printf("J-SPIN!"); break;
                        case MINO_S: printf("S-SPIN!"); break;
                        case MINO_Z: printf("Z-SPIN!"); break;
                        case MINO_T: printf("T-SPIN!"); break;
                        default:     printf("MINO-SPIN!"); break;
                    }
                    printf("\033[0m\n");
                    fflush(stdout);
                    
                    // 스핀 성공 보너스 스코어 가산 (50점 추가)
                    score += 50; 
                }
            }
            /* ────────────────────────────────────────────────────────── */
            // 안전한 빈 자리를 찾았으므로 최종 데이터 반영 후 탈출
            memcpy(current_piece.matrix, next_matrix, sizeof(next_matrix));
            current_piece.x = target_x;
            current_piece.y = target_y;
            current_piece.rotation = next_rot;
            lock_delay_count++; // 회전 성공 시 카운터 증가
            return; 
        }
    }
    // 테스트를 모두 돌았는데도 빈 자리가 없다면 회전은 최종 '불가' 처리되어 회전 입력 무시
}

/* 게임 시작 시 Next 큐를 처음으로 채워주는 함수 */
static void init_next_queue(void) {
    refill_bag(); // 7-Bag 주머니 초기 충전
    for (int i = 0; i < 3; i++) {
        next_queue[i] = pull_next_piece();
    }
}

/* ──────────────── Hold(블록 보관) 알고리즘 ──────────────── */
static void execute_hold_piece(void) {
    if (!can_hold) return; // 이번 턴에 이미 홀드를 사용했다면 명령 무시

    mino_t current_type = current_piece.type;

    // Case 1: 보관함이 완전히 비어있는 경우
    if (hold_queue == MINO_EMPTY) {
        hold_queue = current_type;      // 현재 블록을 보관함에 저장
        spawn_piece_with_next();        // Next 대기열에서 다음 블록을 새로 스폰
    } 
    // Case 2: 이미 보관함에 블록이 들어있어서 서로 맞바꿔야 하는 경우
    else {
        mino_t temp = hold_queue;       // 보관함에 있던 블록 임시 백업
        hold_queue = current_type;      // 현재 블록을 보관함에 저장
        
        // 보관함에서 꺼낸 블록을 조작 블록으로 세팅하고 스폰 좌표 초기화
        current_piece.type = temp;
        current_piece.rotation = 0;
        current_piece.x = 3;
        current_piece.y = 0;
        
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                current_piece.matrix[r][c] = mino_templates[temp][r][c];
            }
        }
    }

    can_hold = 0; // 블록이 바닥에 굳기 전까지 홀드 기능 사용 불가
    lock_delay_count = 0; // 바닥 버티기 카운트 초기화
}

/* ──────────────── (특수 기믹) 하드코어 가속 알고리즘 ──────────────── */
// CPU 사용량에 따라 drop_interval을 조정하는 함수 (게임 루프에서 주기적으로 호출)
// CPU 사용량이 1% 상승할 때마다 드롭 주기를 3,500 마이크로초씩 가속
// 만약 CPU 점유율이 80%라면 -> 500,000 - (80 * 3,500) = 220,000 (약 2.3배 빨라짐)
static void update_system_speed(long *current_interval) {
    FILE *p = popen("sh scripts/sys_monitor.sh 2>/dev/null", "r");
    if (!p) {
        current_cpu_usage = 0;
        *current_interval = base_drop_interval;
        return;
    }

    int cpu = 0;
    if (fscanf(p, "%d", &cpu) == 1) {
        // 백분율이 정상 범위(0~100)일 때만 반영
        if (cpu >= 0 && cpu <= 100) {
            current_cpu_usage = cpu;
        }
    }
    pclose(p);

    long reduction = (long)current_cpu_usage * 3500;
    *current_interval = base_drop_interval - reduction;

    // 게임이 아예 불가능할 정도로 빨라지는 것을 막기 위한 하한선 설정 (최소 0.15초)
    if (*current_interval < 150000) {
        *current_interval = 150000;
    }
}

/* ──────────────── Next 대기열 교체 및 스폰 ──────────────── */
static void spawn_piece_with_next(void) {
    // Next 큐의 가장 첫 번째(0번) 블록을 현재 블록으로 채택!
    mino_t next_type = next_queue[0];
    
    current_piece.type = next_type;
    current_piece.rotation = 0;
    current_piece.x = 3;
    current_piece.y = 0;
    
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            current_piece.matrix[r][c] = mino_templates[next_type][r][c];
        }
    }

    // 대기열을 앞으로 한 칸씩 땡김 (큐의 Shift 연산)
    next_queue[0] = next_queue[1];
    next_queue[1] = next_queue[2];
    
    // 맨 뒤(2번) 빈자리에는 7-Bag에서 새로 하나를 뽑아 채움
    next_queue[2] = pull_next_piece();
    
    can_hold = 1; // 홀드 기회 리셋
    lock_delay_count = 0; // 바닥 버티기 카운트 리셋
}

/* 특정 미노 타입을 받아 4x2 격자 문자열로 우측 화면에 그려주는 조각 함수 */
static void print_next_mino_preview(mino_t type, int row_line) {
    // 해당 미노의 기본 템플릿 모양을 기반으로 출력
    // 터미널 공간을 아끼기 위해 4x4 중 상단 2줄만 압축 출력
    for (int c = 0; c < 4; c++) {
        if (mino_templates[type][row_line][c]) {
            if (type == MINO_I)      printf("%s■ %s", C_I, RST);
            else if (type == MINO_O) printf("%s■ %s", C_O, RST);
            else if (type == MINO_T) printf("%s■ %s", C_T, RST);
            else if (type == MINO_S) printf("%s■ %s", C_S, RST);
            else if (type == MINO_Z) printf("%s■ %s", C_Z, RST);
            else if (type == MINO_L) printf("%s■ %s", C_L, RST);
            else if (type == MINO_J) printf("%s■ %s", C_J, RST);
        } else {
            printf("  "); // 빈칸
        }
    }
}

static void render(void) {
    printf("\033[2J\033[H");

    printf("=========================================" EL "\n");
    printf("   VI-TETRIS : %s님의 도전 (game3)"      EL "\n", player_name);
    printf("=========================================" EL "\n");

    // 임시 그리기 맵 버퍼 생성 (보드 데이터 + 조작중인 블록 합성)
    int display_map[BOARD_ROWS][BOARD_COLS];
    memcpy(display_map, board, sizeof(board));

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (current_piece.matrix[r][c]) {
                int ny = current_piece.y + r;
                int nx = current_piece.x + c;
                if (ny >= 0 && ny < BOARD_ROWS && nx >= 0 && nx < BOARD_COLS) {
                    display_map[ny][nx] = current_piece.type;
                }
            }
        }
    }

    // 화면 렌더링 수행
    for (int r = 0; r < BOARD_ROWS; r++) {
        /* ──────────────── [좌측 HOLD 사이드바 UI 출력 세션] ──────────────── */
        if (r == 1)      printf("  [ HOLD ] ");
        else if (r == 2) { print_next_mino_preview(hold_queue, 0); printf("   "); }
        else if (r == 3) { print_next_mino_preview(hold_queue, 1); printf("   "); }
        else             printf("           "); // 일반 행 공백 유지

        // 가운데 테트리스 보드판 경계선과 맵 그리기
        printf(" ┃ ");
        for (int c = 0; c < BOARD_COLS; c++) {
            int block = display_map[r][c];
            if (block == MINO_EMPTY) printf("%s. %s", C_GR, RST);
            else if (block == MINO_I) printf("%s■ %s", C_I, RST);
            else if (block == MINO_O) printf("%s■ %s", C_O, RST);
            else if (block == MINO_T) printf("%s■ %s", C_T, RST);
            else if (block == MINO_S) printf("%s■ %s", C_S, RST);
            else if (block == MINO_Z) printf("%s■ %s", C_Z, RST);
            else if (block == MINO_L) printf("%s■ %s", C_L, RST);
            else if (block == MINO_J) printf("%s■ %s", C_J, RST);
        }
        printf("┃");

        /* ──────────────── [오른쪽 NEXT 사이드바 UI 출력 세션] ──────────────── */
        if (r == 1)  printf("   [ NEXT 1 ]");
        if (r == 2) { printf("    "); print_next_mino_preview(next_queue[0], 0); }
        if (r == 3) { printf("    "); print_next_mino_preview(next_queue[0], 1); }
        
        if (r == 5)  printf("   [ NEXT 2 ]");
        if (r == 6) { printf("    "); print_next_mino_preview(next_queue[1], 0); }
        if (r == 7) { printf("    "); print_next_mino_preview(next_queue[1], 1); }
        
        if (r == 9)  printf("   [ NEXT 3 ]");
        if (r == 10){ printf("    "); print_next_mino_preview(next_queue[2], 0); }
        if (r == 11){ printf("    "); print_next_mino_preview(next_queue[2], 1); }

        printf(EL "\n"); // 줄 끝 잔상 지우기 매크로 필수 결합
    }

    printf("=========================================" EL "\n");
    printf("  %s%s★ SCORE: %d%s"                     EL "\n", BOLD, ESC "[33m", score, RST);
    printf("  이동: 화살표 키 회전: z/x 홀드: c  종료: q" EL "\n");
    printf("=========================================" EL "\n");
    fflush(stdout);

    printf(" ┌─────────────────────────────────────┐\n");
    printf(" │ [SYSTEM OS RESOURCE MONITOR]        │\n");
    printf(" │  - Host CPU Usage : %3d %%           │\n", current_cpu_usage);
    
    // CPU 부하에 따른 위험도 경고 문구 출력 (비주얼 요소)
    if (current_cpu_usage >= 70) {
        printf(" │  - DANGER : %sOVERCLOCK ACTIVE!%s     │\n", ESC "[5;31m", RST);
    } else if (current_cpu_usage >= 40) {
        printf(" │  - STATUS : %sWARPING SPEED%s         │\n", ESC "[33m", RST);
    } else {
        printf(" │  - STATUS : STABLE LEVEL            │\n");
    }
    printf(" └─────────────────────────────────────┘\n");

    printf(ESC "[J"); 
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc >= 2 && argv[1][0] != '\0') {
        strncpy(player_name, argv[1], sizeof(player_name) - 1);
    }
    
    srand((unsigned int)time(NULL));
    printf(ESC "[2J" ESC "[3J" ESC "[H"); // 첫 진입 시 스크롤백 완전 청소
    
    enable_raw_mode();
    atexit(disable_raw_mode);

    init_next_queue();
    spawn_piece_with_next();
    
    while (!game_over) {
        render();
        
        // 0.5초 동안 유저의 키 입력을 기다림
        if (kbhit_timeout(drop_interval)) {
            char key;
            read(STDIN_FILENO, &key, 1);
            
            /* 방향키 제어부 (Escape Sequence 파싱) */
            if (key == '\033') {
                char seq[2];
                // 뒤이어 들어오는 2바이트('[', 'A~D')를 버퍼에서 신속히 낚아챔
                if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
                    if (seq[0] == '[') {
                        switch (seq[1]) {
                            case 'D': // ← 왼쪽 방향키
                                // 왼쪽으로 한 칸 가도 안전한지 미리 확인!
                                if (!check_collision(current_piece.x - 1, current_piece.y, current_piece.matrix)) {
                                    current_piece.x--; 
                                }
                                break;
                            case 'C': // → 오른쪽 방향키
                                // 오른쪽으로 한 칸 가도 안전한지 미리 확인!
                                if (!check_collision(current_piece.x + 1, current_piece.y, current_piece.matrix)) {
                                    current_piece.x++; 
                                }
                                break;
                            case 'B': // ↓ 아래쪽 방향키 (소프트 드롭)
                                // 아래로 한 칸 가도 안전한지 미리 확인!
                                if (!check_collision(current_piece.x, current_piece.y + 1, current_piece.matrix)) {
                                    current_piece.y++;
                                    score += 1; // 소프트 드롭 보너스 점수 가산
                                }
                                else {
                                    // 소프트 드롭 중 바닥에 부딪히면 즉시 고정
                                    lock_and_clear_lines();
                                }
                                break;
                            case 'A': // ↑ 위쪽 방향키 (시계 방향 회전)
                                execute_srs_rotation(1);
                                break;
                        }
                    }
                }
            }
            /* 일반 알파벳 및 특수 키 제어부 (Z, X, C, Q, Space) */
            else {
                switch (key) {
                    case 'q': case 'Q': 
                        game_over = 1; 
                        break;
                         
                    case 'z': case 'Z': // 반시계 회전 
                        execute_srs_rotation(0); 
                        break;
                        
                    case 'x': case 'X': // 시계 회전 
                        execute_srs_rotation(1); // 1: 시계
                        break;
                        
                    case 'c': case 'C': // 홀드 (Hold)
                        execute_hold_piece(); // 선언해둔 함수 바인딩
                        break;
                        
                    case ' ': // 스페이스바 하드 드롭
                        execute_hard_drop();
                        break;
                }
            }
        } 
        // 0.5초 동안 키 입력이 없었을 때 (타임아웃 발생)
        else {
            // 아래로 한 칸 내려가기 전에 충돌 검사 실행!
            if (!check_collision(current_piece.x, current_piece.y + 1, current_piece.matrix)) {
                current_piece.y++; // 안전하면 한 칸 하강
            } else {
                // 아래가 막혀있다면 바닥 고정 및 라인 클리어 실행!
                lock_and_clear_lines();
            }
        }
    }
    
    trigger_sound("gameover");
    
    disable_raw_mode();
    
    // 부모 로비(`lobby.c`) 파이프 고속도로로 최종 확장 대형 점수 전송
    int pipe_fd = -1;
    if (argc >= 4 && argv[3] && argv[3][0] != '\0') {
        pipe_fd = atoi(argv[3]);
        write(pipe_fd, &score, sizeof(score));
        close(pipe_fd);
    }
    
    return 0;
}