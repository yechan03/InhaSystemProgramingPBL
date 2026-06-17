#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "account.h"

#include <unistd.h>// fork(), execl() 함수 원형, pid_t 자료형 정의가 들어있는 헤더
#include <sys/wait.h>// wait(), WIFEXITED, WEXITSTATUS 매크로가 들어있는 헤더
#include <signal.h>// signal(), SIGINT (ISO C 표준 헤더) - 게임 실행 중 로비의 Ctrl+C 보호용
#include "score.h"

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

/* 화면을 지우고 커서를 좌상단으로. 메뉴를 항상 같은 자리에서 다시 그려
 * 화면이 아래로 흐르지(스크롤) 않게 한다. */
static void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

/* 결과 메시지가 다음 화면 지우기로 사라지기 전에 사용자가 읽도록 잠시 멈춘다. */
static void pause_enter(void) {
    char tmp[16];
    printf("\n계속하려면 Enter 를 누르세요...");
    fflush(stdout);
    read_line(tmp, sizeof(tmp));
}

static void print_main_menu(void) {
    clear_screen();
    printf("\n=========================================\n");
    printf("   Inha SysProg PBL : Mini-Game Lobby\n");
    printf("=========================================\n");
    printf(" 1) 회원가입\n");
    printf(" 2) 로그인\n");
    printf(" 0) 종료\n");
    printf("선택 > ");
}

static int do_register(void) {
    char id[MAX_ID_LEN], pw[MAX_PW_LEN], github[MAX_GH_LEN];

    clear_screen();
    printf("\n=========================================\n");
    printf("            회원가입 (Sign Up)\n");
    printf("=========================================\n");
    printf(" GitHub username 은 실제 존재하는 계정이어야 하며,\n");
    printf(" 영문·숫자·하이픈(-) 으로 1~39자 입니다.\n");
    printf("-----------------------------------------\n");
    printf("  아이디          : "); read_line(id, sizeof(id));
    printf("  GitHub username : "); read_line(github, sizeof(github));
    printf("  비밀번호        : "); read_password(pw, sizeof(pw));
    printf("-----------------------------------------\n");

    int r = account_register(id, pw, github);
    if (r == 0)   { printf("  [OK] 가입 완료! 환영합니다. (GitHub: %s)\n", github); printf("=========================================\n"); return 0;  }
    if (r == -1)  { printf("  [X] 이미 존재하는 아이디입니다.\n"); }
    else if (r == -3)  { printf("  [X] 입력 형식 오류: 아이디/비밀번호/GitHub username 을\n      비우지 말고 username 규칙을 지켜 주세요.\n"); }
    else if (r == -4)  { printf("  [X] GitHub 에 존재하지 않는 사용자입니다.\n      (네트워크 미연결·curl 미설치 시에도 동일하게 표시됩니다.)\n"); }
    else          { printf("  [X] 계정 파일 저장에 실패했습니다.\n"); }
    printf("=========================================\n");
    return -1;
}

static int do_login(char *out_user, size_t un, char *out_github, size_t gn) {
    char id[MAX_ID_LEN], pw[MAX_PW_LEN];

    clear_screen();
    printf("\n=========================================\n");
    printf("             로그인 (Login)\n");
    printf("=========================================\n");
    printf("  아이디    : ");   read_line(id, sizeof(id));
    printf("  비밀번호  : "); read_password(pw, sizeof(pw));
    printf("-----------------------------------------\n");

    if (account_login(id, pw, out_github, gn) == 0) {
        snprintf(out_user, un, "%s", id);
        printf("  [OK] 환영합니다, %s 님! (GitHub: %s)\n", out_user, out_github);
        printf("=========================================\n");
        return 0;
    }
    printf("  [X] 로그인 실패: 아이디 또는 비밀번호가\n      올바르지 않습니다.\n");
    printf("=========================================\n");
    return -1;
}

static void lobby_menu(const char *user, const char *github) {
    while (1) {
        clear_screen();
        printf("\n=========================================\n");
        printf("        INHA ARCADE :  게임 로비\n");
        printf("=========================================\n");
        printf("  사용자 : %s\n", user);
        printf("  GitHub : %s\n", github);
        printf("-----------------------------------------\n");
        printf("  [ 미니게임 ]\n");
        printf("   1) %-17s - VI 던전 탐험 RPG\n",   "VI-RPG");
        printf("   2) %-17s - 깃허브 다마고치 키우기\n", "GitHub Tamagotchi");
        printf("   3) %-17s - 실시간 테트리스\n",     "VI-TETRIS");
        printf("   4) %-17s - 자원 경영 시뮬레이션\n", "Factory-ism");
        printf("   5) %-17s - 기사의 여행\n",         "Knight's Tour");
        printf("-----------------------------------------\n");
        printf("  [ 기타 ]\n");
        printf("   9) 순위표 (Leaderboard)\n");
        printf("   0) 로그아웃\n");
        printf("=========================================\n");
        printf("  선택 > ");

        char buf[16];
        read_line(buf, sizeof(buf));
        int sel = atoi(buf);

        if (sel == 0) {
            printf("\n  [INFO] 로그아웃 되었습니다.\n");
            pause_enter();
            return;
        }
        else if(sel >= 1 && sel <= 5){
            clear_screen();
            printf("\n  [System] Game%d 프로세스를 생성합니다...\n", sel);

            // 부모와 자식 간의 "실행 실패"와 "최종 점수" 공유를 위한 파이프 생성
            int exec_pipe[2];
            if (pipe(exec_pipe) < 0) {
                perror("[X] Pipe 생성 실패");
                continue;
            }
            
            // 자식 프로세스 생성
            pid_t pid = fork(); 

            if (pid < 0) {
                perror("[X] Fork 실패");
                close(exec_pipe[0]);
                close(exec_pipe[1]);
                continue;
            } 
            else if (pid == 0) {
                // =========== 자식 프로세스 영역 ===========
                close(exec_pipe[0]); // 읽기 전용 포트는 닫음
                
                // 부모와 연결된 파이프 쓰기 포트(exec_pipe[1]) 번호를 문자열로 변환
                char pipe_fd_str[16];
                sprintf(pipe_fd_str, "%d", exec_pipe[1]);

                char game_path[32];
                char game_name[16];

                // 실행 파일 경로 규칙 지정 (예: games/game1)
                sprintf(game_path, "games/game%d", sel);
                // game4 는 bash 스크립트(.sh 확장자 관례 유지).
                // #!/bin/bash 셔뱅을 커널이 해석하므로 바이너리와 동일하게 execl 로 실행된다.
                if (sel == 4) strcat(game_path, ".sh");
                sprintf(game_name, "game%d", sel);

                // execl을 사용하여 격리된 공간에서 새 게임 프로그램으로 넘어감
                execl(game_path, game_name, user, github, pipe_fd_str, (char *)NULL);

                // execl이 실패했을 경우
                // 점수는 항상 0 이상이므로 -1 은 "실행 실패" 전용 신호로 안전하다.
                int error_signal = -1;
                // 부모에게 실행 실패 신호(-1)를 파이프로 전송
                write(exec_pipe[1], &error_signal, sizeof(error_signal));
                close(exec_pipe[1]);
                
                perror("[X] 게임 실행 실패");
                exit(-1);
            } 
            else {
                // =========== 부모 프로세스 영역 ===========
                close(exec_pipe[1]); // 쓰기 전용 포트는 닫음

                // 게임이 도는 동안 Ctrl+C(SIGINT)는 자식(게임)만 받도록 로비는 잠시 무시.
                // Ctrl+C 로만 끝나는 게임(game4)에서 로비까지 같이 죽는 것을 방지한다.
                void (*old_sigint)(int) = signal(SIGINT, SIG_IGN);

                /* ── 점수 회수 프로토콜 ──
                 * 자식이 파이프에 쓴 4바이트 int 를 읽는다.
                 *   received_data == -1 : execl 실패 신호 (게임 바이너리 없음)
                 *   received_data >=  0 : 게임이 직접 보낸 최종 점수 (game3/game5, 255점 초과 가능)
                 *   nbytes == 0         : 파이프 미사용 게임 (game1/game2)
                 *                         → 종료코드(WEXITSTATUS, 0~255)에서 점수 회수
                 */
                int received_data = 0;
                int nbytes = read(exec_pipe[0], &received_data, sizeof(received_data));
                close(exec_pipe[0]);

                int status;
                // 자식 프로세스가 종료될 때까지 대기
                wait(&status);

                signal(SIGINT, old_sigint); // 로비의 Ctrl+C 동작 원복

                // 자식이 execl 실패 신호(-1)를 남긴 경우: 게임 바이너리 누락
                if (nbytes > 0 && received_data == -1) {
                    printf("\n=========================================\n");
                    printf("  [X] 게임 프로그램 파일을 실행할 수 없습니다.\n");
                    printf("      scripts/build.sh 로 게임 바이너리를 먼저 생성하세요.\n");
                    printf("=========================================\n");
                }
                else {
                    int game_score = -1;

                    if (nbytes > 0) {
                        // 파이프 점수 방식 (game3/game5): 8bit 제한 없이 큰 점수 그대로 수신
                        game_score = received_data;
                    }
                    else if (sel != 4 && WIFEXITED(status)) {
                        // 종료코드 점수 방식 (game1/game2): exit(score) 를 WEXITSTATUS 로 회수
                        // game4(bash)는 Ctrl+C 종료 시 bash 가 종료코드 130 으로 끝나
                        // 가짜 점수가 기록될 수 있으므로 종료코드 회수 대상에서 제외한다.
                        game_score = WEXITSTATUS(status);
                    }

                    if (game_score >= 0) {
                        printf("\n=========================================\n");
                        printf("  [OK] 게임이 정상 종료되었습니다.\n");
                        printf("  [Result] %s 님의 최종 점수 : %d 점\n", user, game_score);
                        printf("=========================================\n");

                        save_high_score(sel, user, game_score);// 게임이 종료될 때 점수가 기존 최고점수를 넘겼으면 최고점수를 업데이트하는 함수(score.h에 포함)
                    } else if (WIFEXITED(status)) {
                        printf("\n=========================================\n");
                        printf("  [INFO] 게임이 종료되었습니다.\n");
                        printf("=========================================\n");
                    } else {
                        // 파이프에도 안 쓰고 정상 종료도 아님: 시그널 등으로 강제 소멸
                        printf("\n=========================================\n");
                        printf("  [X] 게임 프로세스가 비정상적으로 종료되었습니다.\n");
                        printf("=========================================\n");
                    }
                }
            }
            pause_enter();
        }
        else if(sel == 9){
            show_leaderboard();
            pause_enter();
        }
        else{
            printf("\n  [X] 잘못된 선택입니다. 메뉴의 번호를 입력해 주세요.\n");
            pause_enter();
        }
    }
}

int main(void) {
    char user[MAX_ID_LEN];
    char github[MAX_GH_LEN];
    while (1) {
        print_main_menu();
        char buf[16];
        read_line(buf, sizeof(buf));
        int sel = atoi(buf);

        if (sel == 0) break;
        else if (sel == 1) {
            do_register();
            pause_enter();
        }
        else if (sel == 2) {
            user[0]   = '\0';
            github[0] = '\0';
            if (do_login(user, sizeof(user), github, sizeof(github)) == 0) {
                pause_enter();              /* 환영 메시지를 본 뒤 로비로 */
                lobby_menu(user, github);
            } else {
                pause_enter();              /* 로그인 실패 메시지 확인 */
            }
        }
        else {
            printf("[X] 잘못된 선택입니다.\n");
            pause_enter();
        }
    }
    printf("\n프로그램을 종료합니다. 안녕히 가세요!\n");
    return 0;
}
