#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "account.h"

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

int account_register(const char *id, const char *pw) {
    if (id[0] == '\0' || pw[0] == '\0') return -3;
    if (strchr(id, ':') || strchr(id, '\n')) return -3;
    if (account_exists(id)) return -1;

    FILE *fp = fopen(ACCOUNT_FILE, "a");
    if (!fp) return -2;
    fprintf(fp, "%s:%lu\n", id, hash_credential(id, pw));
    fclose(fp);
    return 0;
}

int account_login(const char *id, const char *pw) {
    FILE *fp = fopen(ACCOUNT_FILE, "r");
    if (!fp) return -1;
    unsigned long target = hash_credential(id, pw);
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        unsigned long stored = strtoul(colon + 1, NULL, 10);
        if (strcmp(line, id) == 0 && stored == target) {
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    return -1;
}
