#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>// 최고점수 기록을 달성한 시간을 측정하기 위해 추가
#include "score.h"
#include "account.h"// 플레이어 개인 데이터를 받아오기 위해 추가

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

/* ───────────────── [인하아케이드 전용 스코어보드 시스템] ───────────────── */
// data/scores.txt에서 데이터를 읽어와 대시보드로 순위표를 출력하는 함수
void show_leaderboard(void) {
    int target_game = 0;
    
    // 전체 게임 리스트를 대시보드 상단에 고정 표출
    printf("\033[2J\033[H"); // 화면 청소 후 좌상단 정렬
    printf("=======================================================\n");
    printf("              INHA ARCADE - GAME LIST                  \n");
    printf("=======================================================\n");
    printf("  [1] Game 1 : (기본 미니게임)\n");
    printf("  [2] Game 2 : GitHub Tamagotchi (다마고치 키우기)\n");
    printf("  [3] Game 3 : VI-TETRIS (하드코어 시스템 테트리스)\n");
    printf("  [4] Game 4 : Management Game (자원 경영 시뮬레이션)\n");
    printf("  [5] Game 5 : Knight's Tour (기사의 여행)\n");
    printf("=======================================================\n");
    printf(" 조회할 게임 번호를 선택하세요 (종료: 0) > ");
    
    // 안전한 정수 입력 예외 처리
    char input_buf[16];
    if (!fgets(input_buf, sizeof(input_buf), stdin)) return;
    if (sscanf(input_buf, "%d", &target_game) != 1 || target_game <= 0 || target_game > 5) {
        printf("\n[INFO] 메뉴 조회가 취소되었거나 메인 화면으로 돌아갑니다.\n");
        return;
    }

    // 파일 열기 시도
    FILE *fp = fopen("data/scores.txt", "r");
    if (!fp) {
        printf("\n[!] 아직 등록된 랭킹/점수 데이터베이스가 존재하지 않습니다.\n");
        return;
    }

    rank_entry_t rank_list[200]; // 최대 200명 레코드 가상 메모리 매핑
    int entry_count = 0;

    int g_num, s_val;
    char u_name[64], t_stamp[32];

    // 파일 전체 오프셋을 스캔하며 유저가 "선택한 게임 번호" 레코드만 메모리에 스트리밍
    // 파일 포맷: game_number:username:high_score:timestamp
    while (fscanf(fp, "%d:%[^:]:%d:%[^\n]\n", &g_num, u_name, &s_val, t_stamp) == 4) {
        if (g_num == target_game) {
            // sizeof 전체를 넘겨 안전하게 복사한 뒤, 맨 끝 버퍼 자리에 Null 마커 수동 삽입
            strncpy(rank_list[entry_count].username, u_name, sizeof(rank_list[entry_count].username));
            rank_list[entry_count].username[sizeof(rank_list[entry_count].username) - 1] = '\0';

            rank_list[entry_count].score = s_val;

            // 타임스탬프 영역도 동일하게 공간 무결성 확보
            strncpy(rank_list[entry_count].timestamp, t_stamp, sizeof(rank_list[entry_count].timestamp));
            rank_list[entry_count].timestamp[sizeof(rank_list[entry_count].timestamp) - 1] = '\0';

            entry_count++;
            
            if (entry_count >= 200) break; // 오버플로우 메모리 락 방지
        }
    }
    fclose(fp);

    if (entry_count == 0) {
        printf("\n%s[!] 선택하신 Game #%d은 아직 획득한 최고점수 기록이 없습니다.%s\n", "\033[31m", target_game, "\033[0m");
        return;
    }

    // 수집된 개별 게임 데이터를 피벗 기반 퀵정렬로 스코어링 내림차순 랭크 셋업
    qsort(rank_list, entry_count, sizeof(rank_entry_t), compare_scores);

    // 순수 등수를 1위부터 연산 출력
    printf("\033[2J\033[H");
    printf("=======================================================\n");
    printf("         ★ INHA ARCADE: GAME #%d LEADERBOARD ★       \n", target_game);
    printf("=======================================================\n");
    printf("  RANK  |    PLAYER ID    |  HIGH SCORE  |     DATE TIME    \n");
    printf("-------------------------------------------------------\n");
    
    for (int i = 0; i < entry_count; i++) {
        // i + 1 수식을 할당하여 리얼타임 단방향 순위 그래프 동적 표출
        printf("   #%02d  | %-15s |  %11d | %s\n", 
               i + 1, 
               rank_list[i].username, 
               rank_list[i].score, 
               rank_list[i].timestamp);
    }
    printf("=======================================================\n");
}
