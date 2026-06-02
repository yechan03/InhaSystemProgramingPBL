#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "account.h"

/* GitHub username 문법: [A-Za-z0-9-], 1~39자, 하이픈으로 시작/끝 금지.
 * 화이트리스트로 사전 검증해서 셸 인자에 안전하게 넘긴다. */
static int valid_github_username(const char *id) {
    size_t len = strlen(id);
    size_t i;
    if (len < 1 || len > 39) return 0;
    if (id[0] == '-' || id[len - 1] == '-') return 0;
    for (i = 0; i < len; i++) {
        char c = id[i];
        int ok = (c >= 'a' && c <= 'z') ||
                 (c >= 'A' && c <= 'Z') ||
                 (c >= '0' && c <= '9') ||
                 (c == '-');
        if (!ok) return 0;
    }
    return 1;
}

/* scripts/github_check.sh 를 system() 으로 호출. 종료코드 0 이면 존재. */
static int github_user_exists(const char *id) {
    char cmd[256];
    /* id 는 위에서 화이트리스트 검증을 거쳤으므로 셸 메타문자가 없다. */
    snprintf(cmd, sizeof(cmd),
        "sh scripts/github_check.sh '%s' >/dev/null 2>&1", id);
    int ret = system(cmd);
    return ret == 0;
}

unsigned long hash_credential(const char *id, const char *pw) {
    unsigned long h = 5381UL;
    int c;
    const char *s = id;
    while ((c = (unsigned char)*s++)) h = ((h << 5) + h) ^ (unsigned long)c;
    h = ((h << 5) + h) ^ (unsigned long)':';
    s = pw;
    while ((c = (unsigned char)*s++)) h = ((h << 5) + h) ^ (unsigned long)c;
    return h;
}

int account_exists(const char *id) {
    FILE *fp = fopen(ACCOUNT_FILE, "r");
    if (!fp) return 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        if (strcmp(line, id) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int account_register(const char *id, const char *pw, const char *github) {
    if (id[0] == '\0' || pw[0] == '\0' || github[0] == '\0') return -3;
    if (strchr(id, ':') || strchr(id, '\n'))         return -3;
    if (strchr(github, ':') || strchr(github, '\n')) return -3;
    if (!valid_github_username(github))              return -3;
    if (account_exists(id))                          return -1;
    if (!github_user_exists(github))                 return -4;

    FILE *fp = fopen(ACCOUNT_FILE, "a");
    if (!fp) return -2;
    fprintf(fp, "%s:%lu:%s\n", id, hash_credential(id, pw), github);
    fclose(fp);
    return 0;
}

int account_login(const char *id, const char *pw, char *github_out, size_t n) {
    FILE *fp = fopen(ACCOUNT_FILE, "r");
    if (!fp) return -1;
    unsigned long target = hash_credential(id, pw);
    char line[256];

    if (github_out && n > 0) github_out[0] = '\0';

    while (fgets(line, sizeof(line), fp)) {
        /* 줄 끝 개행 제거 */
        size_t llen = strlen(line);
        while (llen > 0 && (line[llen - 1] == '\n' || line[llen - 1] == '\r')) {
            line[--llen] = '\0';
        }

        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';

        char *hash_str   = colon + 1;
        char *github_str = NULL;
        char *colon2     = strchr(hash_str, ':');
        if (colon2) {
            *colon2 = '\0';
            github_str = colon2 + 1;
        }

        unsigned long stored = strtoul(hash_str, NULL, 10);
        if (strcmp(line, id) == 0 && stored == target) {
            if (github_out && n > 0) {
                if (github_str && github_str[0]) {
                    snprintf(github_out, n, "%s", github_str);
                } else {
                    /* 구버전 2필드 계정 폴백 : id 를 github 로 가정 */
                    snprintf(github_out, n, "%s", id);
                }
            }
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return -1;
}
