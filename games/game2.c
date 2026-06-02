/*
 * game2 : GitHub Tamagotchi (real-time mirror)
 *
 * 실제 GitHub 공개 이벤트(PushEvent)를 popen() 으로 받아와 다마고치 표정을 결정한다.
 *   - 마지막 commit 으로부터 7일 이상 -> 사망
 *   - 5~6일 -> 빈사,  3~4일 -> 슬픔
 *   - 0일 + 최근 30일 push 가 많을수록 행복 / 매우행복 / 전설
 *
 * 데이터 소스 : scripts/github_stats.sh USER  (stdout 2줄 = days_since / count_30d)
 * 사용 라이브러리 : C 표준 라이브러리만 (stdio/stdlib/string)
 * 실행 인자       : argv[1] = 로비에서 넘어온 사용자 ID (GitHub username)
 * 종료 코드       : 최종 점수 (0~255 clamp). 로비 WEXITSTATUS 회수
 *
 * 게임플레이 : 사실상 GitHub habit 뷰어. 'r' 새로고침 / 'q' 종료.
 * 점수가 갱신되려면 실제로 GitHub 에 commit 을 push 하고 다시 들어와야 한다.
 */

/* popen() / pclose() 는 POSIX 확장이라 -std=c99 모드에서 stdio.h 가 숨긴다.
 * 이 매크로를 stdio.h 전에 정의해야 선언이 노출된다. */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEATH_DAYS   7
#define SAD_DAYS     3
#define HAPPY_COMMITS_30D     5
#define ECSTATIC_COMMITS_30D  20
#define LEGENDARY_COMMITS_30D 50

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
    /* DYING */
    {
        "        ___________",
        "       /           \\",
        "      |   T     T   |",
        "      |      .      |",
        "      |    \\___/    |",
        "       \\___________/"
    },
    /* SAD */
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
    /* HAPPY */
    {
        "        ___________",
        "       /           \\",
        "      |   ^     ^   |",
        "      |      v      |",
        "      |     \\_/     |",
        "       \\___________/"
    },
    /* ECSTATIC */
    {
        "        ___________",
        "       /   *   *   \\",
        "      |   >     <   |",
        "      |      v      |",
        "      |    \\_o_/    |",
        "       \\___________/"
    },
    /* LEGENDARY */
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

static char username[64];
static int  days_since_commit;     /* 999 = 데이터 없음/오프라인 */
static int  total_commits_30d;
static int  fetch_ok;
static char last_msg[160];

static expr_t current_expression(void) {
    if (!fetch_ok)                       return EXPR_NEUTRAL;
    if (days_since_commit >= DEATH_DAYS) return EXPR_DEAD;
    if (days_since_commit >= DEATH_DAYS - 2) return EXPR_DYING;   /* 5,6일 */
    if (days_since_commit >= SAD_DAYS)   return EXPR_SAD;         /* 3,4일 */

    /* 오늘 commit 이 있으면 (days_since_commit == 0) 양에 따라 행복도 상승.
     * 오늘 안 했으면(1,2) NEUTRAL. */
    if (days_since_commit == 0) {
        if (total_commits_30d >= LEGENDARY_COMMITS_30D) return EXPR_LEGENDARY;
        if (total_commits_30d >= ECSTATIC_COMMITS_30D)  return EXPR_ECSTATIC;
        if (total_commits_30d >= HAPPY_COMMITS_30D)     return EXPR_HAPPY;
    }
    return EXPR_NEUTRAL;
}

/* scripts/github_stats.sh 를 popen() 으로 호출해 두 줄을 읽어온다. */
static void fetch_github_state(void) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
        "sh scripts/github_stats.sh '%s' 2>/dev/null", username);

    FILE *p = popen(cmd, "r");
    if (!p) {
        fetch_ok = 0;
        days_since_commit = 999;
        total_commits_30d = 0;
        snprintf(last_msg, sizeof(last_msg),
            "[!] popen 실패 - shell 또는 curl 사용 불가?");
        return;
    }

    int d = 999, c = 0;
    if (fscanf(p, "%d", &d) != 1) d = 999;
    if (fscanf(p, "%d", &c) != 1) c = 0;
    pclose(p);

    days_since_commit = d;
    total_commits_30d = c;
    fetch_ok = (d != 999 || c > 0);   /* 둘 다 기본값이면 실패로 간주 */

    if (!fetch_ok) {
        snprintf(last_msg, sizeof(last_msg),
            "[!] GitHub 데이터를 가져오지 못했습니다. (네트워크/사용자 확인)");
    } else {
        snprintf(last_msg, sizeof(last_msg),
            "[OK] GitHub 상태를 가져왔습니다.");
    }
}

static void clear_screen(void) {
    printf("\033[2J\033[H");
}

static void render(void) {
    expr_t e = current_expression();
    int i;

    clear_screen();
    printf("=========================================\n");
    printf("   GitHub Tamagotchi (game2)\n");
    printf("   - 실 GitHub PushEvent 기반 -\n");
    printf("=========================================\n");
    printf(" Player : %s\n", username);
    printf("-----------------------------------------\n");

    for (i = 0; i < 6; i++) {
        printf("%s\n", faces[e][i]);
    }
    printf("\n");
    printf("       << %s >>\n", expr_label(e));
    printf("\n");

    if (fetch_ok) {
        if (days_since_commit >= 999) {
            printf(" 최근 push 기록 없음\n");
        } else {
            printf(" 마지막 commit 이후    : %d 일\n", days_since_commit);
            if (days_since_commit < DEATH_DAYS) {
                int left = DEATH_DAYS - days_since_commit;
                printf(" 사망까지 남은 일수    : %d 일\n", left);
            } else {
                printf(" 사망까지 남은 일수    : 0 (이미 사망)\n");
            }
        }
        printf(" 최근 30일 PushEvent  : %d 회\n", total_commits_30d);
    } else {
        printf(" GitHub 데이터를 가져오지 못했습니다.\n");
        printf(" (오프라인이거나 username 이 잘못되었을 수 있음)\n");
    }

    printf("-----------------------------------------\n");
    printf(" [r] refresh      (다시 GitHub 조회)\n");
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

static int compute_score(void) {
    if (!fetch_ok) return 0;

    /* 살아있음 보너스 */
    int alive_bonus = (days_since_commit < DEATH_DAYS) ? 100 : 0;

    /* 신선도 (오늘 commit 했을수록 큰 점수) */
    int recency = 0;
    if (days_since_commit < DEATH_DAYS) {
        recency = (DEATH_DAYS - days_since_commit) * 10;     /* 0~70 */
    }

    /* 30일 활동량 */
    int volume = total_commits_30d;
    if (volume > 100) volume = 100;

    int s = alive_bonus + recency + volume;
    if (s < 0)   s = 0;
    if (s > 255) s = 255;
    return s;
}

static void show_summary(int score) {
    expr_t e = current_expression();
    int i;

    printf("\n=========================================\n");
    printf("   GitHub Tamagotchi 결산\n");
    printf("=========================================\n");
    printf(" Player              : %s\n", username);
    if (fetch_ok) {
        printf(" 마지막 commit 이후 : %d 일\n", days_since_commit);
        printf(" 최근 30일 push     : %d 회\n", total_commits_30d);
    } else {
        printf(" GitHub 데이터 없음 (점수 0)\n");
    }
    printf(" Final mood          : %s\n", expr_label(e));
    printf("-----------------------------------------\n");
    for (i = 0; i < 6; i++) printf("%s\n", faces[e][i]);
    printf("\n");

    if (!fetch_ok) {
        printf(" 다음엔 인터넷에 연결한 채로 실행해보세요.\n");
    } else if (days_since_commit >= DEATH_DAYS) {
        printf(" %d일째 commit 이 없어 다마고치가 떠났습니다.\n", days_since_commit);
        printf(" 오늘 한 줄이라도 commit -> push 해주세요!\n");
    } else if (days_since_commit == 0 && total_commits_30d >= LEGENDARY_COMMITS_30D) {
        printf(" 전설의 commit 머신! 다마고치가 황홀해합니다.\n");
    } else if (days_since_commit == 0) {
        printf(" 오늘도 commit 성공. 다마고치가 행복합니다.\n");
    } else {
        printf(" 다마고치가 commit 을 기다리고 있어요.\n");
    }

    printf("=========================================\n");
    printf(" 최종 점수: %d 점\n", score);
    printf("   (alive100 + (7-days)*10 + min(commits30,100))\n");
    printf("=========================================\n");
}

int main(int argc, char **argv) {
    /* 로비 규약 : argv[1] = 로그인 ID, argv[2] = GitHub username.
     * game2 는 GitHub API 를 호출해야 하므로 argv[2] 를 우선 사용한다.
     * argv[2] 가 비어있으면 (단독 실행 등) argv[1] 로 폴백. */
    const char *gh = NULL;
    if (argc >= 3 && argv[2] && argv[2][0] != '\0')      gh = argv[2];
    else if (argc >= 2 && argv[1] && argv[1][0] != '\0') gh = argv[1];

    if (gh) snprintf(username, sizeof(username), "%s", gh);
    else    snprintf(username, sizeof(username), "guest");

    days_since_commit = 999;
    total_commits_30d = 0;
    fetch_ok          = 0;
    last_msg[0]       = '\0';

    /* 시작하자마자 1차 fetch */
    fetch_github_state();

    while (1) {
        render();
        char act = ' ';
        if (!read_action(&act)) break;     /* EOF */

        if (act == 'q' || act == 'Q') break;

        if (act == 'r' || act == 'R') {
            snprintf(last_msg, sizeof(last_msg), "[..] GitHub 다시 조회중...");
            render();
            fetch_github_state();
            continue;
        }

        snprintf(last_msg, sizeof(last_msg),
            "[?] r (refresh) 또는 q (quit) 중 선택.");
    }

    int score = compute_score();
    show_summary(score);
    return score;
}
