#ifndef SCORE_H
#define SCORE_H

#include <stddef.h>   /* size_t */

#define SCORE_FILE "data/scores.txt"

void get_current_time_str(char *buf, size_t max_size);
void save_high_score(int game_num, const char *user, int new_score);
void show_leaderboard(void);

#endif
