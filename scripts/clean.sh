#!/bin/sh
# 빌드 산출물 정리 (data/ 는 보존)
cd "$(dirname "$0")/.."
rm -rf bin
# games/game4.sh 는 빌드 산출물이 아니라 bash 스크립트 원본이므로 지우지 않는다
rm -f games/game1 games/game2 games/game3 games/game5
echo "[OK] bin/ 및 games/ 내부 파일 정리 완료 (data/ 는 유지)"
