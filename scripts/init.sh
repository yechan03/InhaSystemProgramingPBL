#!/bin/sh
# data/ 디렉토리 및 빈 계정 파일 초기화
set -e
cd "$(dirname "$0")/.."

mkdir -p data bin
[ -f data/accounts.txt ] || : > data/accounts.txt
[ -f data/scores.txt ] || : > data/scores.txt

echo "[OK] 초기화 완료"
echo "  - data/accounts.txt"
exho "  - data/scores.txt"
