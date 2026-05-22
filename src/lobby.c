#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "account.h"

static void read_line(char *buf, size_t n) {
    if (!fgets(buf, (int)n, stdin)) { buf[0] = '\0'; return; }
    size_t l = strlen(buf);
    if (l > 0 && buf[l - 1] == '\n') buf[l - 1] = '\0';
}

static void read_password(char *buf, size_t n) {
    int echo_off = (system("stty -echo 2>/dev/null") == 0);
    read_line(buf, n);
    if (echo_off) {
        system("stty echo 2>/dev/null");
        printf("\n");
    }
}

static void print_main_menu(void) {
    printf("\n=========================================\n");
    printf("   Inha SysProg PBL : Mini-Game Lobby\n");
    printf("=========================================\n");
    printf(" 1) 회원가입\n");
    printf(" 2) 로그인\n");
    printf(" 0) 종료\n");
    printf("선택 > ");
}

static int do_register(void) {
    char id[MAX_ID_LEN], pw[MAX_PW_LEN];
    printf("새 아이디 : ");   read_line(id, sizeof(id));
    printf("새 비밀번호 : "); read_password(pw, sizeof(pw));

    int r = account_register(id, pw);
    if (r == 0)   { printf("[OK] 가입이 완료되었습니다.\n");           return 0;  }
    if (r == -1)  { printf("[X] 이미 존재하는 아이디입니다.\n");        return -1; }
    if (r == -3)  { printf("[X] 아이디/비밀번호가 비어있거나 형식 오류.\n"); return -1; }
    printf("[X] 계정 파일 저장 실패.\n");
    return -1;
}

static int do_login(char *out_user, size_t n) {
    char id[MAX_ID_LEN], pw[MAX_PW_LEN];
    printf("아이디 : ");   read_line(id, sizeof(id));
    printf("비밀번호 : "); read_password(pw, sizeof(pw));

    if (account_login(id, pw) == 0) {
        strncpy(out_user, id, n - 1);
        out_user[n - 1] = '\0';
        printf("[OK] 환영합니다, %s 님!\n", out_user);
        return 0;
    }
    printf("[X] 로그인 실패: 아이디 또는 비밀번호가 올바르지 않습니다.\n");
    return -1;
}

static void lobby_menu(const char *user) {
    while (1) {
        printf("\n----- [ 로비 ] 사용자: %s -----\n", user);
        printf(" (미니게임 기능은 팀 협의 후 추가 예정)\n");
        printf(" 0) 로그아웃\n");
        printf("선택 > ");

        char buf[16];
        read_line(buf, sizeof(buf));
        int sel = atoi(buf);

        if (sel == 0) {
            printf("[INFO] 로그아웃 되었습니다.\n");
            return;
        }
        printf("[X] 잘못된 선택입니다.\n");
    }
}

int main(void) {
    char user[MAX_ID_LEN];
    while (1) {
        print_main_menu();
        char buf[16];
        read_line(buf, sizeof(buf));
        int sel = atoi(buf);

        if (sel == 0) break;
        else if (sel == 1) do_register();
        else if (sel == 2) {
            user[0] = '\0';
            if (do_login(user, sizeof(user)) == 0) {
                lobby_menu(user);
            }
        }
        else printf("[X] 잘못된 선택입니다.\n");
    }
    printf("\n프로그램을 종료합니다. 안녕히 가세요!\n");
    return 0;
}
