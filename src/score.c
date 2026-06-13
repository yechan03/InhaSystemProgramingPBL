#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include "score.h"

// 만약 score.h에 정의되어 있지 않을 경우를 대비한 안전장치 정의
#ifndef SCORE_FILE
#define SCORE_FILE "data/scores.txt"
#endif

#ifndef MAX_ID_LEN
#define MAX_ID_LEN 64
#endif

//  1. 게임별 점수 데이터를 임시로 담아두기 위한 구조체 정의
typedef struct {
    int g_num;
    char u_id[MAX_ID_LEN];
    int s_val;
    char t_val[30];
} ScoreRecord;

//  2. 순위표 화면에서 실시간 방향키 입력을 감지하기 위한 내부 함수
static int read_leaderboard_key(void) {
    struct termios oldt, newt;
    int ch;
    
    // 터미널 설정을 백업하고 실시간 입력 모드(버퍼링/에코 제거)로 변경
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    ch = getchar();
    
    // 리눅스 화살표 키 특수 이스케이프 시퀀스(\033[...) 파싱
    if (ch == 27) { 
        ch = getchar();
        if (ch == '[') {
            ch = getchar();
            // 원상복구 후 방향키 코드 리턴 ('C' = 우측화살표, 'D' = 좌측화살표)
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            return ch; 
        }
    }
    
    // 원상복구 후 엔터 등 일반 키 리턴
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// 최고 점수를 파일에 저장하는 기존 유지 함수
void save_high_score(int game_num, const char *user, int new_score) {
    char lines[100][256];
    int line_count = 0;
    int found = 0;
    
    FILE *fp = fopen(SCORE_FILE, "r");
    if (fp) {
        while (fgets(lines[line_count], sizeof(lines[0]), fp)) {
            int g_num, s_val;
            char u_id[MAX_ID_LEN];
            char t_val[30];
            
            if (sscanf(lines[line_count], "%d:%[^:]:%d:%[^\n]", &g_num, u_id, &s_val, t_val) == 4) {
                // 내 아이디와 게임 번호가 일치하는 기존 기록 발견 시
                if (g_num == game_num && strcmp(u_id, user) == 0) {
                    found = 1;
                    if (new_score > s_val) {
                        time_t t = time(NULL);
                        struct tm *tm_info = localtime(&t);
                        char time_str[30];
                        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
                        
                        // 최고 기록 갱신
                        sprintf(lines[line_count], "%d:%s:%d:%s\n", game_num, user, new_score, time_str);
                        printf("\n[Record] ★ 축하합니다! 신규 최고 점수가 등록되었습니다! ★\n");
                    } else {
                        printf("\n[Record] 기존 최고 점수(%d점)보다 낮아 기록을 갱신하지 않았습니다.\n", s_val);
                    }
                }
            }
            line_count++;
        }
        fclose(fp);
    }
    
    // 기존에 내 아이디로 등록된 게임 기록이 없었다면 새로 한 줄 추가
    if (!found) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char time_str[30];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        
        sprintf(lines[line_count], "%d:%s:%d:%s\n", game_num, user, new_score, time_str);
        line_count++;
        printf("\n[Record] ★ 생애 첫 게임 플레이! 신규 점수가 등록되었습니다! ★\n");
    }
    
    // 파일에 다시 덮어쓰기 저장
    fp = fopen(SCORE_FILE, "w");
    if (fp) {
        for (int i = 0; i < line_count; i++) {
            fputs(lines[i], fp);
        }
        fclose(fp);
    } else {
        perror("[X] 점수 파일 오픈 실패");
    }
}

//  3. [수정 완료] 게임별로 대시보드를 나누어 방향키로 연동하는 순위표 함수
void show_leaderboard(void) {
    ScoreRecord records[1000];
    int total_records = 0;

    // 파일에서 전체 데이터 딱 한 번 긁어와 배열에 캐싱
    FILE *fp = fopen(SCORE_FILE, "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp) && total_records < 1000) {
            int g_num, s_val;
            char u_id[MAX_ID_LEN];
            char t_val[30];
            
            if (sscanf(line, "%d:%[^:]:%d:%[^\n]", &g_num, u_id, &s_val, t_val) == 4) {
                records[total_records].g_num = g_num;
                snprintf(records[total_records].u_id, MAX_ID_LEN, "%s", u_id);
                records[total_records].s_val = s_val;
                snprintf(records[total_records].t_val, 30, "%s", t_val);
                total_records++;
            }
        }
        fclose(fp);
    }

    int current_game = 1; // 로비 진입 시 기본 1번 게임 페이지 고정

    while (1) {
        printf("\033[2J\033[H"); // 화면 깜빡임 최소화 전체 클리어
        
        printf("\n=========================================================\n");
        printf("               INHA ARCADE LEADERBOARD                 \n");
        printf("=========================================================\n");
        printf("               현재 점수판: [ GAME %d ] \n", current_game);
        printf("---------------------------------------------------------\n");
        printf(" GAME |   PLAYER ID    |   HIGH SCORE  |      DATE TIME    \n");
        printf("---------------------------------------------------------\n");

        int has_record = 0;
        
        // 캐싱된 배열 전체를 탐색하며 오직 현재 'current_game' 번호 데이터만 필터링하여 매핑
        for (int i = 0; i < total_records; i++) {
            if (records[i].g_num == current_game) {
                printf("  #%d  | %-14s |   %-10d | %s\n", 
                       records[i].g_num, records[i].u_id, records[i].s_val, records[i].t_val);
                has_record = 1;
            }
        }

        if (!has_record) {
            printf("        아직 등록된 Game %d의 순위 기록이 없습니다.      \n", current_game);
        }

        printf("==========================================================\n");
        printf(" [◀] 이전 게임  |  [▶] 다음 게임  |  [Enter] 로비로 복귀\n");
        printf("==========================================================\n");
        fflush(stdout);

        // 키 상호작용 감지
        int key = read_leaderboard_key();
        
        if (key == 'C') { // 오른쪽 화살표 키 입력 시 다음 게임으로 이동
            current_game++;
            if (current_game > 5) current_game = 1; // 5번 게임을 초과하면 1번으로 무한 순환
        } 
        else if (key == 'D') { // 왼쪽 화살표 키 입력 시 이전 게임으로 이동
            current_game--;
            if (current_game < 1) current_game = 5; // 1번 게임 미만으로 떨어지면 5번으로 무한 순환
        } 
        else if (key == '\n' || key == '\r') { // 엔터 키 감지 시 루프를 부수고 완전히 탈출
            break;
        }
    }
}