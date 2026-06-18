
#define _XOPEN_SOURCE 500   /* usleep() 사용을 위해 필요 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

/* ──────────────── ANSI 색상 코드 ──────────────── */
#define ESC      "\033"
#define R        ESC "[31m"
#define BR       ESC "[91m"
#define G        ESC "[32m"
#define BG       ESC "[92m"
#define Y        ESC "[33m"
#define BY       ESC "[93m"
#define C_CYAN   ESC "[36m"
#define BC       ESC "[96m"
#define M        ESC "[35m"
#define BM       ESC "[95m"
#define W        ESC "[37m"
#define BW       ESC "[97m"
#define GR       ESC "[90m"
#define BOLD     ESC "[1m"
#define RST      ESC "[0m"
#define BG_Y     ESC "[43m"

/* ──────────────── 화면 제어 ──────────────── */
#define CLEAR_ALL()    printf(ESC "[2J" ESC "[H")
#define HIDE_CURSOR()  printf(ESC "[?25l")
#define SHOW_CURSOR()  printf(ESC "[?25h")
#define GOTO(r, c)     printf(ESC "[%d;%dH", (r), (c))
#define CLEAR_LINE()   printf(ESC "[K")

/* ──────────────── 게임 상수 ──────────────── */
#define PLAYER_MAX_HP   5
#define PLAYER_ATK      2
#define GOBLIN_MAX_HP   3
#define BOSS_MAX_HP     10
#define GOLD_REWARD     5
#define MAP_ROWS        13
#define MAP_COLS        21

/* 점수: 0~255 범위 (WEXITSTATUS 제한) */
#define SCORE_GOBLIN        10
#define SCORE_BOSS          80
#define SCORE_LEVELUP       15
#define SCORE_CLEAR_BONUS   50
#define SCORE_HP_BONUS      5
#define SCORE_TURN_PENALTY  1

/* 화면 좌표 (행 번호)
 * 전체 UI 를 23행 안에 압축해 표준 24행 터미널에서도 스크롤이 일어나지 않게 한다.
 * (바닥 행 너머로 \n 이 밀려나면 화면 전체가 위로 스크롤되어
 *  GOTO 절대 좌표 기반의 맵 갱신과 어긋나 화면 고정이 깨진다.) */
#define MAP_TOP        5                      /* 헤더 1~4행 바로 아래 */
#define MAP_LEFT       3
#define ROW_SEP        (MAP_TOP + MAP_ROWS)   /* 18: 맵 아래 구분선 */
#define ROW_TURN       19
#define ROW_STAT       20                     /* Lv/ATK/Gold/Kill + SCORE 통합 */
#define ROW_BOSS       21                     /* BOSS + 고블린 HP 통합 */
#define ROW_HELP       22
#define ROW_MSG        23

/* ──────────────── 전역 상태 ──────────────── */
static char board[MAP_ROWS][MAP_COLS];
static int  goblin_hp[MAP_ROWS][MAP_COLS];
static int  boss_hp_grid[MAP_ROWS][MAP_COLS];

static int  player_row = 0, player_col = 0;
static int  prev_player_row = 0, prev_player_col = 0;
static int  player_hp     = PLAYER_MAX_HP;
static int  player_attack = PLAYER_ATK;
static int  player_gold   = 0;
static int  player_level  = 1;
static int  turn          = 0;
static int  kill_count    = 0;
static char last_msg[256] = "";
static int  skill_hits    = 0;
static int  boss_dead     = 0;
static int  score         = 0;
static char player_name[64] = "player";

/* 변경된 칸 추적 (최대 64개) */
static struct { int r, c; } dirty_tiles[64];
static int dirty_count = 0;

/* 레벨업 테이블: kills_needed, atk_bonus */
static const int level_table[3][2] = {
    {1, 1}, {3, 1}, {6, 2}
};

/* ──────────────── 터미널 설정 ──────────────── */
static struct termios orig_termios;

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    SHOW_CURSOR();
    fflush(stdout);
}

static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/* ──────────────── 키 입력 ──────────────── */
static char read_key(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) return c;
    return 0;
}

/* ──────────────── dirty 마킹 ──────────────── */
static void mark_dirty(int r, int c) {
    if (dirty_count < 64) {
        dirty_tiles[dirty_count].r = r;
        dirty_tiles[dirty_count].c = c;
        dirty_count++;
    }
}

/* ──────────────── 맵 생성 ──────────────── */
static void generate_map(void) {
    int r, c;

    /* 기본 맵: 외곽은 벽, 내부는 빈칸 */
    for (r = 0; r < MAP_ROWS; r++) {
        for (c = 0; c < MAP_COLS; c++) {
            if (r == 0 || r == MAP_ROWS-1 || c == 0 || c == MAP_COLS-1)
                board[r][c] = '#';
            else
                board[r][c] = '.';
            goblin_hp[r][c] = 0;
            boss_hp_grid[r][c] = 0;
        }
    }

    /* 장애물 랜덤 배치 */
    int obstacles = rand() % 11 + 15;
    int placed = 0, attempts = 0;
    while (placed < obstacles && attempts < 500) {
        r = rand() % (MAP_ROWS - 2) + 1;
        c = rand() % (MAP_COLS - 2) + 1;
        if (board[r][c] == '.') {
            int rnd = rand() % 4;
            if (rnd <= 1)      board[r][c] = '#';
            else if (rnd == 2) board[r][c] = 'T';
            else               board[r][c] = 't';
            placed++;
        }
        attempts++;
    }

    /* 플레이어 배치 (왼쪽 상단 첫 빈칸) */
    player_row = -1; player_col = -1;
    for (r = 1; r < MAP_ROWS-1 && player_row == -1; r++) {
        for (c = 1; c < MAP_COLS-1 && player_row == -1; c++) {
            if (board[r][c] == '.') {
                player_row = r;
                player_col = c;
            }
        }
    }
    prev_player_row = player_row;
    prev_player_col = player_col;

    /* 캐릭터/아이템 배치 함수 */
    char chars_to_place[] = {'H', 'H', '1', '2', '3', 'B'};
    int counts = sizeof(chars_to_place) / sizeof(chars_to_place[0]);
    int i;
    for (i = 0; i < counts; i++) {
        char ch = chars_to_place[i];
        attempts = 0;
        while (attempts < 1000) {
            r = rand() % (MAP_ROWS - 2) + 1;
            c = rand() % (MAP_COLS - 2) + 1;
            if (board[r][c] == '.' && !(r == player_row && c == player_col)) {
                board[r][c] = ch;
                if (ch == '1' || ch == '2' || ch == '3')
                    goblin_hp[r][c] = GOBLIN_MAX_HP;
                if (ch == 'B')
                    boss_hp_grid[r][c] = BOSS_MAX_HP;
                break;
            }
            attempts++;
        }
    }
}

/* ──────────────── 몬스터 HP 찾기 ──────────────── */
static int find_char_hp(char ch) {
    int r, c;
    for (r = 0; r < MAP_ROWS; r++) {
        for (c = 0; c < MAP_COLS; c++) {
            if (board[r][c] == ch) {
                if (ch == 'B') return boss_hp_grid[r][c];
                return goblin_hp[r][c];
            }
        }
    }
    return 0;
}

/* ──────────────── HP 바 출력 ──────────────── */
static void print_hp_bar(int cur, int max, const char *col) {
    int filled = cur * 10 / max;
    if (filled < 0) filled = 0;
    if (filled > 10) filled = 10;

    printf("[");
    int i;
    for (i = 0; i < 10; i++) {
        if (i < filled) printf("%s█%s", col, RST);
        else            printf("%s░%s", GR, RST);
    }
    printf("]");
}

/* ──────────────── 한 타일 그리기 ──────────────── */
static void draw_tile_at(int r, int c, int hi) {
    int screen_row = MAP_TOP + r;
    int screen_col = MAP_LEFT + c * 2;
    GOTO(screen_row, screen_col);

    /* 플레이어 자리 */
    if (r == player_row && c == player_col) {
        printf("%s%s♥%s", BOLD, BY, RST);
        return;
    }

    /* 스킬 강조 */
    if (hi) {
        printf("%s%s%s*%s", BG_Y, BOLD, BR, RST);
        return;
    }

    char t = board[r][c];
    switch (t) {
        case '#': printf("%s█%s", GR, RST); break;
        case 'T': printf("%s♣%s", G,  RST); break;
        case 't': printf("%s♠%s", BG, RST); break;
        case 'H': printf("%sH%s", BC, RST); break;
        case '1': printf("%s1%s", BR, RST); break;
        case '2': printf("%s2%s", BR, RST); break;
        case '3': printf("%s3%s", BR, RST); break;
        case 'B': printf("%s%sB%s", BOLD, M, RST); break;
        case '.': printf("%s.%s", GR, RST); break;
        default:  printf("%c", t);
    }
}

/* ──────────────── 전체 화면 1회 그리기 ──────────────── */
static void draw_full_screen(void) {
    CLEAR_ALL();
    /* 헤더 (1~4행). 이후 모든 출력은 GOTO 절대 좌표만 사용해
     * 바닥 행에서 \n 으로 인한 화면 스크롤(=고정 깨짐)을 차단한다. */
    printf("  %s%s────────────────────────────────────────────────%s\n", BOLD, C_CYAN, RST);
    printf("  %s%s       >>>  VI-RPG : %s님의 모험  <<<%s\n", BOLD, BY, player_name, RST);
    printf("  %s%s────────────────────────────────────────────────%s\n", BOLD, C_CYAN, RST);
    printf("  %s%s[ 게임 맵 ]%s", BOLD, W, RST);

    int r, c;
    for (r = 0; r < MAP_ROWS; r++) {
        for (c = 0; c < MAP_COLS; c++) {
            draw_tile_at(r, c, 0);   /* GOTO 로 절대 좌표에 직접 그림 */
        }
    }

    /* 하단 푸터 */
    GOTO(ROW_SEP, 1);
    printf("  %s%s────────────────────────────────────────────────%s", BOLD, C_CYAN, RST);
    GOTO(ROW_HELP, 1);
    printf("  이동:%sw/a/s/d%s  공격:%sSPACE%s  스킬:%se%s  종료:%sq%s",
           BY, RST, BY, RST, BY, RST, BY, RST);

    fflush(stdout);
}

/* ──────────────── 상태바 갱신 ──────────────── */
static void draw_status(void) {
    const char *hp_col = (player_hp > 2) ? BG : BR;

    /* TURN/HP 줄 */
    GOTO(ROW_TURN, 1); CLEAR_LINE();
    printf("  %sTURN %d%s  HP:%s%d%s/%d ", BOLD, turn, RST, hp_col, player_hp, RST, PLAYER_MAX_HP);
    print_hp_bar(player_hp, PLAYER_MAX_HP, hp_col);

    /* Lv/ATK/Gold/Kill + SCORE 줄 (24행 터미널에 맞춰 한 줄로 통합) */
    GOTO(ROW_STAT, 1); CLEAR_LINE();
    printf("  %sLv:%d%s  %sATK:%d%s  %sGold:%d%s  %sKill:%d%s  %s%s★ SCORE: %d%s",
           Y, player_level, RST, BY, player_attack, RST,
           Y, player_gold, RST, GR, kill_count, RST,
           BOLD, BC, score, RST);

    /* BOSS + 고블린 HP 줄 (한 줄로 통합) */
    int bhp = find_char_hp('B');
    const char *b_col = (bhp > BOSS_MAX_HP / 2) ? M : BR;
    int g1 = find_char_hp('1');
    int g2 = find_char_hp('2');
    int g3 = find_char_hp('3');
    GOTO(ROW_BOSS, 1); CLEAR_LINE();
    printf("  %s%sBOSS%s %s%d%s/%d ", BOLD, M, RST, b_col, bhp, RST, BOSS_MAX_HP);
    print_hp_bar(bhp, BOSS_MAX_HP, b_col);
    printf("  %sG1:%d/%d%s %sG2:%d/%d%s %sG3:%d/%d%s",
           BR, g1, GOBLIN_MAX_HP, RST,
           BR, g2, GOBLIN_MAX_HP, RST,
           BR, g3, GOBLIN_MAX_HP, RST);
}

/* ──────────────── 메시지 줄 갱신 ──────────────── */
static void draw_message(void) {
    GOTO(ROW_MSG, 1); CLEAR_LINE();
    if (last_msg[0] != '\0') {
        printf("%s", last_msg);
    }
}

/* ──────────────── 변경된 부분만 다시 그리기 ──────────────── */
static void flush_dirty(void) {
    /* 플레이어 이동: 이전 위치 + 새 위치 */
    if (prev_player_row != player_row || prev_player_col != player_col) {
        int save_pr = player_row, save_pc = player_col;
        player_row = -1; player_col = -1;
        draw_tile_at(prev_player_row, prev_player_col, 0);
        player_row = save_pr; player_col = save_pc;

        draw_tile_at(player_row, player_col, 0);
        prev_player_row = player_row;
        prev_player_col = player_col;
    }

    /* 추가 변경 등록된 칸들 */
    int i;
    for (i = 0; i < dirty_count; i++) {
        draw_tile_at(dirty_tiles[i].r, dirty_tiles[i].c, 0);
    }
    dirty_count = 0;

    draw_status();
    draw_message();
    fflush(stdout);
}

/* ──────────────── 이동 ──────────────── */
static void do_move(char key) {
    int nr = player_row, nc = player_col;
    switch (key) {
        case 'w': case 'W': nr--; break;
        case 's': case 'S': nr++; break;
        case 'a': case 'A': nc--; break;
        case 'd': case 'D': nc++; break;
        default: return;
    }
    if (nr < 0 || nr >= MAP_ROWS || nc < 0 || nc >= MAP_COLS) return;

    char tile = board[nr][nc];
    if (tile == '#' || tile == 'T' || tile == 't') return;

    if (tile == '1' || tile == '2' || tile == '3') {
        player_hp--;
        snprintf(last_msg, sizeof(last_msg),
                 " %s! 고블린에게 피해! HP-1%s", BR, RST);
    } else if (tile == 'B') {
        player_hp -= 2;
        snprintf(last_msg, sizeof(last_msg),
                 " %s!! 보스에게 피해! HP-2%s", BR, RST);
    } else if (tile == 'H') {
        int heal = PLAYER_MAX_HP - player_hp;
        if (heal > GOLD_REWARD) heal = GOLD_REWARD;
        if (heal > 0) {
            player_hp += heal;
            snprintf(last_msg, sizeof(last_msg),
                     " %s+ 상점에서 HP %d 회복!%s", BC, heal, RST);
        } else {
            snprintf(last_msg, sizeof(last_msg),
                     " %s이미 HP가 최대입니다.%s", GR, RST);
        }
    }

    prev_player_row = player_row;
    prev_player_col = player_col;
    player_row = nr;
    player_col = nc;
}

/* ──────────────── 일반 공격 (상하좌우 4방향) ──────────────── */
static void do_attack(void) {
    int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    int i;
    for (i = 0; i < 4; i++) {
        int r = player_row + dirs[i][0];
        int c = player_col + dirs[i][1];
        if (r < 0 || r >= MAP_ROWS || c < 0 || c >= MAP_COLS) continue;

        char tile = board[r][c];
        if (tile == '1' || tile == '2' || tile == '3') {
            if (goblin_hp[r][c] > 0) {
                goblin_hp[r][c] -= player_attack;
                if (goblin_hp[r][c] < 0) goblin_hp[r][c] = 0;
            }
        } else if (tile == 'B') {
            if (boss_hp_grid[r][c] > 0) {
                boss_hp_grid[r][c] -= player_attack;
                if (boss_hp_grid[r][c] < 0) boss_hp_grid[r][c] = 0;
            }
        }
    }
}

/* ──────────────── 범위 스킬 (3x3) ──────────────── */
static void do_skill(void) {
    skill_hits = 0;
    int skill_dmg = player_attack / 2;
    if (skill_dmg < 1) skill_dmg = 1;

    int dr, dc;

    /* 1단계: 범위 강조 */
    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int r = player_row + dr;
            int c = player_col + dc;
            if (r < 0 || r >= MAP_ROWS || c < 0 || c >= MAP_COLS) continue;
            draw_tile_at(r, c, 1);
        }
    }
    fflush(stdout);
    usleep(300000);  /* 0.3초 */

    /* 2단계: 데미지 + dirty 등록 */
    for (dr = -1; dr <= 1; dr++) {
        for (dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;
            int r = player_row + dr;
            int c = player_col + dc;
            if (r < 0 || r >= MAP_ROWS || c < 0 || c >= MAP_COLS) continue;

            char tile = board[r][c];
            if (tile == '1' || tile == '2' || tile == '3') {
                if (goblin_hp[r][c] > 0) {
                    goblin_hp[r][c] -= skill_dmg;
                    if (goblin_hp[r][c] < 0) goblin_hp[r][c] = 0;
                    skill_hits++;
                }
            } else if (tile == 'B') {
                if (boss_hp_grid[r][c] > 0) {
                    boss_hp_grid[r][c] -= skill_dmg;
                    if (boss_hp_grid[r][c] < 0) boss_hp_grid[r][c] = 0;
                    skill_hits++;
                }
            }
            mark_dirty(r, c);
        }
    }
}

/* ──────────────── 죽은 몬스터 처리 + 점수 ──────────────── */
static void check_dead(void) {
    boss_dead = 0;
    int r, c;
    for (r = 0; r < MAP_ROWS; r++) {
        for (c = 0; c < MAP_COLS; c++) {
            char tile = board[r][c];
            if (tile == '1' || tile == '2' || tile == '3') {
                if (goblin_hp[r][c] <= 0) {
                    board[r][c] = '.';
                    player_gold += GOLD_REWARD;
                    kill_count++;
                    score += SCORE_GOBLIN;
                    mark_dirty(r, c);
                }
            } else if (tile == 'B') {
                if (boss_hp_grid[r][c] <= 0) {
                    board[r][c] = '.';
                    player_gold += GOLD_REWARD;
                    kill_count++;
                    score += SCORE_BOSS;
                    boss_dead = 1;
                    mark_dirty(r, c);
                }
            }
        }
    }
}

/* ──────────────── 레벨업 체크 ──────────────── */
static int check_levelup(void) {
    int lv_idx = player_level - 1;
    if (lv_idx < 0 || lv_idx >= 3) return 0;
    int needed = level_table[lv_idx][0];
    int bonus  = level_table[lv_idx][1];
    if (kill_count >= needed) {
        player_level++;
        player_attack += bonus;
        score += SCORE_LEVELUP;
        return 1;
    }
    return 0;
}

/* ──────────────── 최종 점수 계산 ──────────────── */
static void finalize_score(void) {
    int turn_penalty = turn * SCORE_TURN_PENALTY;
    score -= turn_penalty;
    if (boss_dead) {
        score += SCORE_CLEAR_BONUS;
        score += player_hp * SCORE_HP_BONUS;
    }
    if (score < 0) score = 0;
    if (score > 255) score = 255;
}

/* ──────────────── 결과 화면 ──────────────── */
static void show_result(const char *result) {
    CLEAR_ALL();
    SHOW_CURSOR();
    printf("\n");
    if (strcmp(result, "CLEAR") == 0) {
        printf("  %s%s+========================================+%s\n", BOLD, BY, RST);
        printf("  %s%s|          *** GAME CLEAR ***            |%s\n", BOLD, BY, RST);
        printf("  %s%s+========================================+%s\n", BOLD, BY, RST);
    } else if (strcmp(result, "DEAD") == 0) {
        printf("  %s%s+========================================+%s\n", BOLD, BR, RST);
        printf("  %s%s|          --- GAME OVER ---             |%s\n", BOLD, BR, RST);
        printf("  %s%s+========================================+%s\n", BOLD, BR, RST);
    } else {
        printf("  %s%s+========================================+%s\n", BOLD, GR, RST);
        printf("  %s%s|          -- 게임 종료 --                |%s\n", BOLD, GR, RST);
        printf("  %s%s+========================================+%s\n", BOLD, GR, RST);
    }
    printf("\n");
    printf("  플레이어 : %s\n", player_name);
    printf("  킬 수    : %d\n", kill_count);
    printf("  레벨     : %d\n", player_level);
    printf("  남은 HP  : %d\n", player_hp);
    printf("  플레이턴: %d\n", turn);
    printf("  %s%s최종 점수: %d%s\n", BOLD, BY, score, RST);
    printf("\n");
    printf("  %s엔터를 누르면 로비로 돌아갑니다...%s\n", BY, RST);
    fflush(stdout);

    /* 엔터 대기 (raw 모드 끄고) */
    disable_raw_mode();
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
}

/* ──────────────── 타이틀 화면 ──────────────── */
static void show_title(void) {
    CLEAR_ALL();
    SHOW_CURSOR();
    printf("\n");
    printf("  %s%s╔══════════════════════════════════════════╗%s\n", BOLD, BY, RST);
    printf("  %s%s║         VI-RPG : VI RPG 던전             ║%s\n", BOLD, BY, RST);
    printf("  %s%s╚══════════════════════════════════════════╝%s\n", BOLD, BY, RST);
    printf("\n");
    printf("  %s플레이어: %s%s\n", BY, player_name, RST);
    printf("\n");
    printf("  %s┌─────────────────────────────────────────┐%s\n", C_CYAN, RST);
    printf("  %s│  이동: w(위) a(왼) s(아래) d(오)        │%s\n", C_CYAN, RST);
    printf("  %s│  공격: SPACE  스킬: e  종료: q          │%s\n", C_CYAN, RST);
    printf("  %s├─────────────────────────────────────────┤%s\n", C_CYAN, RST);
    printf("  %s│  ♥=나  1/2/3=고블린  B=보스  H=상점     │%s\n", C_CYAN, RST);
    printf("  %s│  ♣/♠=나무  █=벽  .=이동가능             │%s\n", C_CYAN, RST);
    printf("  %s├─────────────────────────────────────────┤%s\n", C_CYAN, RST);
    printf("  %s│  [점수] 고블린+10  보스+80  레벨업+15   │%s\n", C_CYAN, RST);
    printf("  %s│  클리어 +50, 남은HP×5, 턴당 -1          │%s\n", C_CYAN, RST);
    printf("  %s│  (최대 255점)                           │%s\n", C_CYAN, RST);
    printf("  %s└─────────────────────────────────────────┘%s\n", C_CYAN, RST);
    printf("\n");
    printf("  %s엔터를 누르면 시작합니다...%s\n", BY, RST);
    fflush(stdout);

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
}

/* ──────────────── 메인 게임 루프 ──────────────── */
static void game_loop(void) {
    generate_map();
    player_hp     = PLAYER_MAX_HP;
    player_attack = PLAYER_ATK;
    player_gold   = 0;
    player_level  = 1;
    turn          = 0;
    kill_count    = 0;
    score         = 0;
    last_msg[0]   = '\0';
    dirty_count   = 0;

    HIDE_CURSOR();
    draw_full_screen();
    draw_status();
    draw_message();
    fflush(stdout);

    enable_raw_mode();

    while (1) {
        char key = read_key();
        last_msg[0] = '\0';

        if (key == 'q' || key == 'Q') {
            finalize_score();
            disable_raw_mode();
            show_result("QUIT");
            break;
        }

        if (key == 'w' || key == 'W' || key == 'a' || key == 'A' ||
            key == 's' || key == 'S' || key == 'd' || key == 'D') {
            do_move(key);
        }
        else if (key == ' ') {
            do_attack();
            check_dead();
            snprintf(last_msg, sizeof(last_msg),
                     " %s>> 공격!%s", BY, RST);
            if (check_levelup()) {
                snprintf(last_msg, sizeof(last_msg),
                         " %s%s*** LEVEL UP! Lv%d ***%s",
                         BOLD, BY, player_level, RST);
            }
            if (boss_dead) {
                flush_dirty();
                finalize_score();
                disable_raw_mode();
                show_result("CLEAR");
                break;
            }
        }
        else if (key == 'e' || key == 'E') {
            do_skill();
            check_dead();
            snprintf(last_msg, sizeof(last_msg),
                     " %s%s** 범위 스킬! %d칸 적중! **%s",
                     BOLD, C_CYAN, skill_hits, RST);
            if (check_levelup()) {
                snprintf(last_msg, sizeof(last_msg),
                         " %s%s*** LEVEL UP! Lv%d ***%s",
                         BOLD, BY, player_level, RST);
            }
            if (boss_dead) {
                flush_dirty();
                finalize_score();
                disable_raw_mode();
                show_result("CLEAR");
                break;
            }
        }

        turn++;
        flush_dirty();

        if (player_hp <= 0) {
            finalize_score();
            disable_raw_mode();
            show_result("DEAD");
            break;
        }
    }
}

/* ──────────────── 진입점 ──────────────── */
int main(int argc, char *argv[]) {
    /* 플레이어 이름 (lobby.c가 execl로 넘김) */
    if (argc > 1 && argv[1][0] != '\0') {
        strncpy(player_name, argv[1], sizeof(player_name) - 1);
        player_name[sizeof(player_name) - 1] = '\0';
    }

    /* 랜덤 시드 */
    srand((unsigned int)time(NULL));

    /* 비정상 종료 시에도 터미널 복구 */
    atexit(disable_raw_mode);

    show_title();
    game_loop();

    /* 점수를 exit code로 반환 (0~255) → WEXITSTATUS로 받음 */
    return score;
}
