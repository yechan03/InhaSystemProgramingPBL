#!/bin/sh
# C 소스 빌드 (표준 라이브러리만 사용)
set -e
cd "$(dirname "$0")/.."

mkdir -p bin data games
sh scripts/init.sh >/dev/null

CC=${CC:-gcc}
CFLAGS="-std=c99 -Wall -Wextra -O2 -Isrc"

SRC="src/lobby.c src/account.c src/score.c"

echo "[BUILD] $CC $CFLAGS"
$CC $CFLAGS $SRC -o bin/lobby
echo "[OK] bin/lobby 생성 완료"

# 2. 게임 빌드 산출물 위치
if [ -f games/game1.c ]; then
    $CC $CFLAGS games/game1.c -o games/game1
    echo "[OK] games/game1 생성 완료"
fi
if [ -f games/game2.c ]; then
    $CC $CFLAGS games/game2.c -o games/game2
    echo "[OK] games/game2 생성 완료"
fi
if [ -f games/game3.c ]; then
    $CC $CFLAGS games/game3.c -o games/game3
    echo "[OK] games/game3 생성 완료"
fi
if [ -f games/game5.c ]; then
    $CC $CFLAGS games/game5.c -o games/game5
    echo "[OK] games/game5 생성 완료"
fi

# 3. game4 는 bash 스크립트: execl 실행에 필요한 +x 권한 보장
if [ -f games/game4.sh ]; then
    chmod +x games/game4.sh
    echo "[OK] games/game4.sh 실행 권한 확인 완료"
fi