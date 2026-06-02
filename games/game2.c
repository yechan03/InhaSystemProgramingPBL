/*
 * game2 : GitHub Tamagotchi (다마고치)
 *
 * 매일 commit 을 해줘야 다마고치가 행복하게 산다.
 *   - 3일 이상 미 commit  : 표정이 점점 슬퍼짐
 *   - 7일 연속 미 commit  : 사망 (게임 오버)
 *   - 연속 commit 이 쌓일수록 표정이 좋아짐
 *
 * 사용 라이브러리 : C 표준 라이브러리만 (stdio / stdlib / string)
 * 실행 인자       : argv[1] = 로비에서 넘어온 사용자 ID (선택)
 * 종료 코드       : 최종 점수 (0~255 clamp). 로비에서 WEXITSTATUS 로 회수
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HP_MAX            10
#define MOOD_MAX          10
#define DEATH_DAYS         7  /* 7일 연속 미 commit -> 사망 */
#define SAD_DAYS           3  /* 3일 미 commit 부터 슬픔 */
#define HAPPY_STREAK       3
#define ECSTATIC_STREAK    7
#define LEGENDARY_STREAK  14

typedef enum {
    EXPR_DEAD = 0,
    EXPR_DYING,
    EXPR_SAD,
    EXPR_NEUTRAL,
    EXPR_HAPPY,
    EXPR_ECSTATIC,
    EXPR_LEGENDARY,
    EXPR_COUNT
} expr_t;

/* 표정별 6줄짜리 얼굴 ASCII 아트 */
static const char *faces[EXPR_COUNT][6] = {
    /* DEAD */
    {
        "        ___________",
        "       /           \\",
        "      |   X     X   |",
        "      |      .      |",
        "      |    -----    |",
        "       \\___________/"
    },
    /* DYING : 5~6일 미 commit */
    {
        "        ___________",
        "       /           \\",
        "      |   T     T   |",
        "      |      .      |",
        "      |    \\___/    |",
        "       \\___________/"
    },
    /* SAD : 3~4일 미 commit */
    {
        "        ___________",
        "       /           \\",
        "      |   u     u   |",
        "      |      v      |",
        "      |     ___     |",
        "       \\___________/"
    },
    /* NEUTRAL */
    {
        "        ___________",
        "       /           \\",
        "      |   o     o   |",
        "      |      v      |",
        "      |     ---     |",
        "       \\___________/"
    },
    /* HAPPY : streak >= 3 */
    {
        "        ___________",
        "       /           \\",
        "      |   ^     ^   |",
        "      |      v      |",
        "      |     \\_/     |",
        "       \\___________/"
    },
    /* ECSTATIC : streak >= 7 */
    {
        "        ___________",
        "       /   *   *   \\",
        "      |   >     <   |",
        "      |      v      |",
        "      |    \\_o_/    |",
        "       \\___________/"
    },
    /* LEGENDARY : streak >= 14 */
    {
        "        ___________",
        "       / \\(^o^)/   \\",
        "      |   *     *   |",
        "      |      v      |",
        "      |   <\\___/>   |",
        "       \\___________/"
    }
};

static const char *expr_label(expr_t e) {
    switch (e) {
        case EXPR_DEAD:      return "R.I.P. (사망)";
        case EXPR_DYING:     return "다 죽어가는 중...";
        case EXPR_SAD:       return "슬픔";
        case EXPR_NEUTRAL:   return "그럭저럭";
        case EXPR_HAPPY:     return "행복";
        case EXPR_ECSTATIC:  return "매우 행복";
        case EXPR_LEGENDARY: return "*** 전설 ***";
        default:             return "?";
    }
}

static int  alive;
static int  hp;
static int  mood;
static int  days_since_commit;     /* 마지막 commit 이후 며칠 지났는가 */
static int  consecutive_commits;   /* 현재 연속 commit streak */
static int  total_commits;
static int  max_streak;
static int  days_lived;            /* 살아있는 동안 지난 일수 */
static int  current_day;
static char username[64];
static char last_msg[160];

static expr_t current_expression(void) {
    if (!alive) return EXPR_DEAD;

    /* 미 commit 일수가 우선 (절체절명 상황) */
    if (days_since_commit >= DEATH_DAYS - 2) return EXPR_DYING;   /* 5,6일 */
    if (days_since_commit >= SAD_DAYS)       return EXPR_SAD;     /* 3,4일 */

    /* streak 에 따른 행복도 */
    if (consecutive_commits >= LEGENDARY_STREAK) return EXPR_LEGENDARY;
    if (consecutive_commits >= ECSTATIC_STREAK)  return EXPR_ECSTATIC;
    if (consecutive_commits >= HAPPY_STREAK)     return EXPR_HAPPY;
    return EXPR_NEUTRAL;
}

static void clear_screen(void) {
    /* ANSI : 화면 클리어 + 커서 홈 */
    printf("\033[2J\033[H");
}

static void draw_bar(int val, int max) {
    int filled = (max > 0) ? (val * 10) / max : 0;
    int i;
    if (filled < 0)  filled = 0;
    if (filled > 10) filled = 10;
    printf("[");
    for (i = 0; i < 10; i++) {
        printf("%s", i < filled ? "#" : "-");
    }
    printf("]");
}

static void render(void) {
    expr_t e = current_expression();
    int i;

    clear_screen();
    printf("=========================================\n");
    printf("   GitHub Tamagotchi (game2)\n");
    printf("=========================================\n");
    printf(" Player : %s\n", username);
    printf(" Day    : %d   (생존 %d일)\n", current_day, days_lived);
    printf("-----------------------------------------\n");

    for (i = 0; i < 6; i++) {
        printf("%s\n", faces[e][i]);
    }
    printf("\n");
    printf("       << %s >>\n", expr_label(e));
    printf("\n");

    printf(" HP     : ");
    draw_bar(hp, HP_MAX);
    printf(" (%d/%d)\n", hp, HP_MAX);

    printf(" Mood   : ");
    draw_bar(mood, MOOD_MAX);
    printf(" (%d/%d)\n", mood, MOOD_MAX);

    printf(" Streak : %d 일 연속 commit (best %d)\n",
           consecutive_commits, max_streak);
    printf(" Total  : %d commits\n", total_commits);
    printf(" 마지막 commit 이후: %d일", days_since_commit);

    if (alive && days_since_commit >= SAD_DAYS) {
        int left = DEATH_DAYS - days_since_commit;
        if (left > 0) printf("   [!] 사망까지 %d일", left);
    }
    printf("\n");

    printf("-----------------------------------------\n");
    printf(" [c] commit       (streak +1, 표정 회복)\n");
    printf(" [s] skip         (미 commit 일수 +1)\n");
    printf(" [q] quit         (현재 점수로 종료)\n");
    printf("-----------------------------------------\n");
    if (last_msg[0]) {
        printf(" %s\n", last_msg);
        printf("-----------------------------------------\n");
    }
    printf(" 선택 > ");
    fflush(stdout);
}

static int read_action(char *out) {
    char buf[32];
    size_t i;
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    for (i = 0; buf[i]; i++) {
        if (buf[i] != ' ' && buf[i] != '\t' &&
            buf[i] != '\n' && buf[i] != '\r') {
            *out = buf[i];
            return 1;
        }
    }
    *out = ' ';
    return 1;
}

/* 반환값: 1 = 하루 진행, 0 = 진행 없음(무효 입력), -1 = 즉시 종료 */
static int handle_action(char act) {
    switch (act) {
        case 'c': case 'C':
            total_commits++;
            consecutive_commits++;
            days_since_commit = 0;
            if (consecutive_commits > max_streak)
                max_streak = consecutive_commits;

            mood += 2;
            if (mood > MOOD_MAX) mood = MOOD_MAX;
            hp += 1;
            if (hp > HP_MAX) hp = HP_MAX;

            if (consecutive_commits >= LEGENDARY_STREAK) {
                snprintf(last_msg, sizeof(last_msg),
                    "[c] commit! streak %d 일... 다마고치가 전설이 되어가요!",
                    consecutive_commits);
            } else if (consecutive_commits >= ECSTATIC_STREAK) {
                snprintf(last_msg, sizeof(last_msg),
                    "[c] commit! streak %d 일! 다마고치가 매우 행복합니다.",
                    consecutive_commits);
            } else if (consecutive_commits >= HAPPY_STREAK) {
                snprintf(last_msg, sizeof(last_msg),
                    "[c] commit! streak %d 일. 다마고치가 좋아합니다.",
                    consecutive_commits);
            } else {
                snprintf(last_msg, sizeof(last_msg),
                    "[c] commit! (streak %d)", consecutive_commits);
            }
            return 1;

        case 's': case 'S':
            consecutive_commits = 0;
            days_since_commit++;
            mood -= 2;
            if (mood < 0) mood = 0;
            hp -= 1;
            if (hp < 0) hp = 0;

            if (days_since_commit >= DEATH_DAYS) {
                snprintf(last_msg, sizeof(last_msg),
                    "[X] %d일 연속 미 commit... 다마고치가 떠났습니다.",
                    days_since_commit);
            } else if (days_since_commit >= DEATH_DAYS - 2) {
                snprintf(last_msg, sizeof(last_msg),
                    "[!] %d일째 commit 없음. 사망까지 %d일 남았어요...",
                    days_since_commit, DEATH_DAYS - days_since_commit);
            } else if (days_since_commit >= SAD_DAYS) {
                snprintf(last_msg, sizeof(last_msg),
                    "[s] skip. %d일째 commit 없음. 다마고치가 슬퍼합니다.",
                    days_since_commit);
            } else {
                snprintf(last_msg, sizeof(last_msg),
                    "[s] skip. %d일째 commit 없음.",
                    days_since_commit);
            }
            return 1;

        case 'q': case 'Q':
            return -1;

        default:
            snprintf(last_msg, sizeof(last_msg),
                "[?] c (commit), s (skip), q (quit) 중에 선택하세요.");
            return 0;
    }
}

/* 하루를 넘긴 뒤 사망 여부 판정 */
static void advance_day(void) {
    days_lived++;
    current_day++;
    if (alive && days_since_commit >= DEATH_DAYS) {
        alive = 0;
    }
}

static void show_summary(int final_score) {
    expr_t e = alive ? current_expression() : EXPR_DEAD;
    int i;

    printf("\n=========================================\n");
    printf("   GitHub Tamagotchi 결산\n");
    printf("=========================================\n");
    printf(" Player        : %s\n", username);
    printf(" Days lived    : %d 일\n", days_lived);
    printf(" Total commits : %d\n", total_commits);
    printf(" Max streak    : %d 일\n", max_streak);
    printf(" Final mood    : %s\n", expr_label(e));
    printf("-----------------------------------------\n");
    for (i = 0; i < 6; i++) printf("%s\n", faces[e][i]);
    printf("\n");
    if (!alive) {
        printf(" 다마고치는 %d일 만에 떠났습니다.\n", days_lived);
        printf(" 매일 commit 을 잊지 마세요!\n");
    } else if (max_streak >= LEGENDARY_STREAK) {
        printf(" 전설이 된 다마고치와 행복한 결말!\n");
    } else if (max_streak >= ECSTATIC_STREAK) {
        printf(" 다마고치가 매우 만족스러워 합니다.\n");
    } else {
        printf(" 다마고치를 살아 있게 지키는데 성공했습니다.\n");
    }
    printf("=========================================\n");
    printf(" 최종 점수: %d 점\n", final_score);
    printf("   (commits*2 + max_streak*3 + days_lived)\n");
    printf("=========================================\n");
}

int main(int argc, char **argv) {
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        snprintf(username, sizeof(username), "%s", argv[1]);
    } else {
        snprintf(username, sizeof(username), "guest");
    }

    alive               = 1;
    hp                  = HP_MAX;
    mood                = MOOD_MAX / 2;
    days_since_commit   = 0;
    consecutive_commits = 0;
    total_commits       = 0;
    max_streak          = 0;
    days_lived          = 0;
    current_day         = 1;
    last_msg[0]         = '\0';

    while (alive) {
        render();

        char act = ' ';
        if (!read_action(&act)) break;     /* EOF */

        int r = handle_action(act);
        if (r < 0) break;                  /* quit */
        if (r > 0) advance_day();          /* 하루 진행 */
        /* r == 0 이면 같은 날 다시 입력 */
    }

    /* 점수 = commits*2 + max_streak*3 + days_lived (0~255 clamp) */
    int score = total_commits * 2 + max_streak * 3 + days_lived;
    if (score < 0)   score = 0;
    if (score > 255) score = 255;

    show_summary(score);
    return score;
}
