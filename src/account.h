#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stddef.h>   /* size_t */

#define MAX_ID_LEN 32
#define MAX_PW_LEN 64
#define MAX_GH_LEN 40   /* GitHub username 최대 39자 + NUL */
#define ACCOUNT_FILE "data/accounts.txt"

/* accounts.txt 포맷 : id:hash:github\n
 *  - id     : 로비 로그인 ID (자유 형식, ':' / '\n' 만 금지)
 *  - hash   : djb2 XOR (id + ':' + pw) hash
 *  - github : GitHub username (게임 등에서 사용)
 *
 * 구버전(2필드 `id:hash\n`) 계정은 로그인 시 github 가 비어있고,
 * 그 경우 id 자체를 github 로 폴백한다.
 */

int account_register(const char *id, const char *pw, const char *github);
int account_login(const char *id, const char *pw, char *github_out, size_t n);
int account_exists(const char *id);

unsigned long hash_credential(const char *id, const char *pw);

#endif
