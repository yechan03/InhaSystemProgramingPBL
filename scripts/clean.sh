#!/bin/sh
# 빌드 산출물 정리 (data/ 는 보존)
cd "$(dirname "$0")/.."
rm -rf bin
echo "[OK] bin/ 정리 완료 (data/ 는 유지)"
