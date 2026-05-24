#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "account.h"

#include <unistd.h>// fork(), execl() 함수 원형이 들어있는 헤더
#include <sys/types.h>// pid_t 자료형 정의가 들어있는 헤더
#include <sys/wait.h>// wait(), WIFEXITED, WEXITSTATUS 매크로가 들어있는 헤더
// 세 헤더 모두 강의노트에서 사용한 헤더(수업시간에 물어보고 사용 불가능하다고 하면 수정할 것)
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
        printf(" 1) Game1(Dummy)");
        printf(" 9) High Score Leader Board (순위표)\n");
        printf(" 0) 로그아웃\n");
        printf("선택 > ");

        char buf[16];
        read_line(buf, sizeof(buf));
        int sel = atoi(buf);

        if (sel == 0) {
            printf("[INFO] 로그아웃 되었습니다.\n");
            return;
        }
        else if(sel == 1){
            printf("[System] 게임 프로세스를 생성합니다...\n");

            // 부모와 자식 간의 실행 실패 공유를 위한 파이프 생성
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
                
                char game_path[32];
                char game_name[16];
                
                // 실행 파일 경로 규칙 지정 (예: games/game1)
                sprintf(game_path, "games/game%d", sel);
                sprintf(game_name, "game%d", sel);

                // execl을 사용하여 격리된 공간에서 새 게임 프로그램으로 넘어감
                // 플레이어 ID를 넘겨서 게임 내에서 개인 데이터를 인식
                execl(game_path, game_name, user, NULL);

                // execl이 실패했을 경우 
                int error_signal = 1;
                // 부모에게 실행 실패 신호(1)를 파이프로 전송
                write(exec_pipe[1], &error_signal, sizeof(error_signal));
                close(exec_pipe[1]);
                
                perror("[X] 게임 실행 실패");
                exit(-1);
            } 
            else {
                // =========== 부모 프로세스 영역 ===========
                close(exec_pipe[1]); // 쓰기 전용 포트는 닫음

                int error_signal = 0;
                // 자식이 execl에 실패하여 파이프에 값을 썼는지 확인
                int nbytes = read(exec_pipe[0], &error_signal, sizeof(error_signal));
                close(exec_pipe[0]);
                
                int status;
                // 자식 프로세스가 종료될 때까지 대기
                wait(&status); 

                while (getchar() != '\n' && getchar() != EOF); 

                // 파이프를 통해 자식이 execl에 실패한 것이 확인된 경우
                if (nbytes > 0 && error_signal == 1) {
                    printf("\n[X] 오류: 게임 프로그램 파일이 존재하지 않거나 실행할 수 없습니다.\n");
                    printf("[INFO] scripts/build.sh 를 실행하여 게임 바이너리를 생성하세요.\n");
                } 
                // 자식이 정상적으로 execl을 거쳐 게임을 플레이하고 종료된 경우
                else if (WIFEXITED(status)) {
                    // 게임이 exit(score)로 남긴 점수를 가져오기
                    int game_score = WEXITSTATUS(status);
                    printf("\n=========================================\n");
                    printf("[OK] 게임이 정상 종료되었습니다.\n");
                    printf("[Result] %s 님의 최종 획득 점수: %d 점\n", user, game_score);
                    printf("=========================================\n");
                    
                    save_high_score(sel, user, game_score);// 게임이 종료될 때 점수가 기존 최고점수를 넘겼으면 최고점수를 업데이트하는 함수(score.h에 포함)
                } else {
                    printf("\n[X] 경고: 게임 프로세스가 비정상적으로 종료되었습니다.\n");
                }
            }
        }
        else if(sel == 9){
            show_leaderboard();// leader board를 보여주는 함수(score.h에 포함)
        }
        else{
            printf("[X] 잘못된 선택입니다.\n");
        }
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
