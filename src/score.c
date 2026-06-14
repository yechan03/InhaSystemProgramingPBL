#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>      // 최고점수 기록을 달성한 시간을 측정하기 위해 추가
#include <unistd.h>
#include <termios.h>
#include "score.h"
#include "account.h"   // 플레이어 개인 데이터를 받아오기 위해 추가

// 랭킹 정렬을 위해 내림차순 정렬할 임시 구조체 배열 정의
typedef struct {
    char username[64];
    int score;
    char timestamp[32];
} rank_entry_t;

// C 표준 라이브러리 qsort 연동용 점수 내림차순 비교 함수
static int compare_scores(const void *a, const void *b) {
    rank_entry_t *entryA = (rank_entry_t *)a;
    rank_entry_t *entryB = (rank_entry_t *)b;
    return entryB->score - entryA->score; // 높은 점수가 위로 정렬
}

// 현재 시스템 시간을 "YYYY-MM-DD HH:MM:SS" 형식의 문자열로 구하는 함수
void get_current_time_str(char *buf, size_t max_size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, max_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// 점수가 획득되었을 때 기존 최고 점수와 비교하여 갱신하거나 새로 저장하는 함수
void save_high_score(int game_num, const char *user, int new_score) {
    FILE *fp = fopen(SCORE_FILE, "r");
    char lines[100][256];
    int line_count = 0;
    int updated = 0;
    char time_str[30];
    
    get_current_time_str(time_str, sizeof(time_str));

    // 기존 파일이 존재하면 읽어서 탐색 및 비교
    if (fp) {
        while (fgets(lines[line_count], sizeof(lines[0]), fp)) {
            int g_num, s_val;
            char u_id[MAX_ID_LEN];
            char t_val[30];
            
            // 데이터 파싱 (게임번호:유저ID:점수:시간)
            if (sscanf(lines[line_count], "%d:%[^:]:%d:%[^\n]", &g_num, u_id, &s_val, t_val) == 4) {
                // 해당 유저의 해당 게임 기록을 찾은 경우
                if (g_num == game_num && strcmp(u_id, user) == 0) {
                    if (new_score > s_val) {
                        // 최고 점수 갱신
                        sprintf(lines[line_count], "%d:%s:%d:%s\n", game_num, user, new_score, time_str);
                        printf("[Record] 축하합니다! 최고 점수가 갱신되었습니다!\n");
                    } else {
                        printf("[Record] 기존 최고 점수(%d점)를 넘지 못했습니다.\n", s_val);
                    }
                    updated = 1;
                }
            }
            line_count++;
        }
        fclose(fp);
    }

    // 만약 해당 유저의 기존 기록이 아예 없었다면 새로운 라인으로 추가
    if (!updated) {
        sprintf(lines[line_count], "%d:%s:%d:%s\n", game_num, user, new_score, time_str);
        line_count++;
        printf("[Record] 신규 최고 점수가 등록되었습니다!\n");
    }

    // 최종 데이터를 파일에 다시 안전하게 덮어쓰기 저장
    fp = fopen(SCORE_FILE, "w");
    if (!fp) {
        perror("[X] 점수 파일 저장 실패");
        return;
    }
    for (int i = 0; i < line_count; i++) {
        fputs(lines[i], fp);
    }
    fclose(fp);
}

// 실시간 방향키 입력을 받기 위한 내부 함수 (안전하게 제어 모드 해제 보장)
static int read_leaderboard_key(void) {
    struct termios oldt, newt;
    int ch;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    ch = getchar();
    if (ch == 27) { // 이스케이프 시퀀스 파싱 (방향키 체크)
        ch = getchar();
        if (ch == '[') {
            ch = getchar();
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return ch; // 'C' -> 오른쪽 화살표, 'D' -> 왼쪽 화살표
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch; // '\n' (엔터) 등 일반 키 리턴
}

/* ───────────────── [인하아케이드 전용 스코어보드 시스템] ───────────────── */
// 방향키로 게임별 대시보드를 넘겨보며 qsort 내림차순 정렬을 출력하는 함수
void show_leaderboard(void) {
    int target_game = 1; // 기본적으로 Game 1 페이지부터 노출 시작

    // 메인 인터랙션 무한 루프 개시
    while (1) {
        // 파일 열기 시도 (루프 돌 때마다 최신 파일 상태 반영)
        FILE *fp = fopen(SCORE_FILE, "r");
        
        rank_entry_t rank_list[200]; // 최대 200명 레코드 가상 메모리 매핑
        int entry_count = 0;

        if (fp) {
            int g_num, s_val;
            char u_name[64], t_stamp[32];

            // 파일 전체 오프셋을 스캔하며 현재 페이지인 "target_game" 레코드만 메모리에 파싱 스트리밍
            while (fscanf(fp, "%d:%[^:]:%d:%[^\n]\n", &g_num, u_name, &s_val, t_stamp) == 4) {
                if (g_num == target_game) {
                    strncpy(rank_list[entry_count].username, u_name, sizeof(rank_list[entry_count].username));
                    rank_list[entry_count].username[sizeof(rank_list[entry_count].username) - 1] = '\0';

                    rank_list[entry_count].score = s_val;

                    strncpy(rank_list[entry_count].timestamp, t_stamp, sizeof(rank_list[entry_count].timestamp));
                    rank_list[entry_count].timestamp[sizeof(rank_list[entry_count].timestamp) - 1] = '\0';

                    entry_count++;
                    if (entry_count >= 200) break; // 오버플로우 방지 락
                }
            }
            fclose(fp);
        }

        // 수집된 개별 게임 데이터를 피벗 기반 퀵정렬로 스코어링 내림차순 랭크 셋업
        if (entry_count > 0) {
            qsort(rank_list, entry_count, sizeof(rank_entry_t), compare_scores);
        }

        // 렌더링 파트
        printf("\033[2J\033[H"); // 화면 청소 후 좌상단 복귀
        printf("============================================================\n");
        printf("           ★ INHA ARCADE: GAME #%d LEADERBOARD ★       \n", target_game);
        printf("============================================================\n");
        printf("  RANK  |    PLAYER ID    |  HIGH SCORE  |      DATE TIME    \n");
        printf("------------------------------------------------------------\n");
        
        if (!fp || entry_count == 0) {
            printf("\n%s        아직 등록된 Game #%d의 최고점수 기록이 없습니다.%s\n\n", "\033[31m", target_game, "\033[0m");
        } else {
            for (int i = 0; i < entry_count; i++) {
                printf("   #%02d  | %-15s |  %11d | %s\n", 
                       i + 1, 
                       rank_list[i].username, 
                       rank_list[i].score, 
                       rank_list[i].timestamp);
            }
        }
        printf("============================================================\n");
        printf(" [◀] 이전 게임   |   [▶] 다음 게임   |   [Enter] 로비로 복귀\n");
        printf("============================================================\n");
        fflush(stdout);

        // 키 제어 분기 처리
        int key = read_leaderboard_key();
        if (key == 'C') { // 오른쪽 방향키
            target_game++;
            if (target_game > 5) target_game = 1; // 5번 넘어가면 1번으로 순환
        } 
        else if (key == 'D') { // 왼쪽 방향키
            target_game--;
            if (target_game < 1) target_game = 5; // 1번 미만으로 떨어지면 5번으로 순환
        } 
        else if (key == '\n' || key == '\r') { // 엔터 키 입력 시 메인 로비 메뉴로 완전 탈출
            break;
        }
    }
}