#!/bin/sh
# 빌드 산출물 정리 (data/ 는 보존)
cd "$(dirname "$0")/.."
rm -rf bin
rm -f games/game1
echo "[OK] bin/ 및 games/ 내부 파일 정리 완료 (data/ 는 유지)"
