#ifndef ACCOUNT_H
#define ACCOUNT_H

#define MAX_ID_LEN 32
#define MAX_PW_LEN 64
#define ACCOUNT_FILE "data/accounts.txt"

int account_register(const char *id, const char *pw);
int account_login(const char *id, const char *pw);
int account_exists(const char *id);

unsigned long hash_credential(const char *id, const char *pw);

#endif
