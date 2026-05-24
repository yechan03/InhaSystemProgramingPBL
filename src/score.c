#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>// 최고점수 기록을 달성한 시간을 측정하기 위해 추가
#include "score.h"
#include "account.h"// 플레이어 개인 데이터를 받아오기 위해 추가

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

// data/scores.txt에서 데이터를 읽어와 예쁜 대시보드로 순위표를 출력하는 함수
void show_leaderboard(void) {
    FILE *fp = fopen(SCORE_FILE, "r");
    printf("\n=======================================================\n");
    printf("               INHA ARCADE LEADERBOARD                 \n");
    printf("=======================================================\n");
    printf(" GAME |   PLAYER ID    |  HIGH SCORE  |     DATE TIME    \n");
    printf("-------------------------------------------------------\n");

    if (!fp) {
        printf("       아직 등록된 게임 순위 기록이 없습니다.          \n");
        printf("=======================================================\n");
        return;
    }

    char line[256];
    int has_record = 0;
    while (fgets(line, sizeof(line), fp)) {
        int g_num, s_val;
        char u_id[MAX_ID_LEN];
        char t_val[30];
        
        if (sscanf(line, "%d:%[^:]:%d:%[^\n]", &g_num, u_id, &s_val, t_val) == 4) {
            // C 표준 printf 서식 지정을 사용해 격자 배치 정렬 출력
            printf("  #%d  | %-14s |   %-10d | %s\n", g_num, u_id, s_val, t_val);
            has_record = 1;
        }
    }
    fclose(fp);
    
    if(!has_record) {
        printf("       출력할 수 있는 정상 데이터 기록이 없습니다.    \n");
    }
    printf("=======================================================\n");
}
