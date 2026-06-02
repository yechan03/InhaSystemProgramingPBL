#!/bin/sh
# scripts/github_check.sh USERNAME
# GitHub 사용자가 실제로 존재하는지 확인한다.
#   exit 0 : 존재함 (HTTP 200)
#   exit 1 : 존재하지 않음 (HTTP 404)
#   exit 2 : 도구 부재 / 네트워크 오류 / 인자 누락
#
# C 측에서는 system() 으로 호출하여 종료코드만 검사한다.

USER="$1"

if [ -z "$USER" ]; then
    echo "[github_check] username required" >&2
    exit 2
fi

if ! command -v curl >/dev/null 2>&1; then
    echo "[github_check] curl not installed" >&2
    exit 2
fi

CODE=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 \
    "https://api.github.com/users/$USER" 2>/dev/null)

case "$CODE" in
    200) exit 0 ;;
    404) exit 1 ;;
    *)
        echo "[github_check] unexpected HTTP $CODE" >&2
        exit 2
        ;;
esac
