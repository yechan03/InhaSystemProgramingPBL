#!/bin/sh
# 로비 실행 (없으면 자동 빌드)
set -e
cd "$(dirname "$0")/.."

if [ ! -x bin/lobby ]; then
    sh scripts/build.sh
fi

mkdir -p data
exec bin/lobby
