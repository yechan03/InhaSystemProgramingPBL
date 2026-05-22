#!/bin/sh
# C 소스 빌드 (표준 라이브러리만 사용)
set -e
cd "$(dirname "$0")/.."

mkdir -p bin data
sh scripts/init.sh >/dev/null

CC=${CC:-gcc}
CFLAGS="-std=c99 -Wall -Wextra -O2 -Isrc"

SRC="src/lobby.c src/account.c"

echo "[BUILD] $CC $CFLAGS"
$CC $CFLAGS $SRC -o bin/lobby
echo "[OK] bin/lobby 생성 완료"
