#!/bin/sh
# scripts/github_stats.sh USERNAME
# GitHub 공개 이벤트(API)에서 PushEvent 들을 긁어 다마고치 상태 계산용 수치를 출력한다.
#
# stdout (반드시 2줄, 숫자만):
#   line 1: 마지막 PushEvent 로부터 지난 일수 (없으면 999)
#   line 2: 응답에 포함된 PushEvent 총 개수 (대략 최근 90일/300건 한도)
#
# C 측에서는 popen() 으로 호출하여 두 줄을 fscanf 한다.
# 오류 시에도 기본값(999 / 0)을 출력해 호출 측 파싱이 망가지지 않게 한다.

USER="$1"
DEFAULT_DAYS=999
DEFAULT_COUNT=0

emit_default() {
    echo "$DEFAULT_DAYS"
    echo "$DEFAULT_COUNT"
}

if [ -z "$USER" ]; then
    echo "[github_stats] username required" >&2
    emit_default
    exit 2
fi

if ! command -v curl >/dev/null 2>&1; then
    echo "[github_stats] curl not installed" >&2
    emit_default
    exit 2
fi

EVENTS=$(curl -s --max-time 5 \
    "https://api.github.com/users/$USER/events/public" 2>/dev/null)

if [ -z "$EVENTS" ]; then
    echo "[github_stats] empty response" >&2
    emit_default
    exit 1
fi

# JSON 을 콤마/괄호 기준으로 줄단위로 풀어 PushEvent 다음 created_at 만 추출한다.
# GitHub 응답은 { "type":"PushEvent", ..., "created_at":"YYYY-MM-DD..." } 구조라
# type 이 created_at 보다 먼저 등장한다는 사실에 의존한다.
DATES=$(printf '%s' "$EVENTS" | tr ',{}[]' '\n' | awk '
    /"type":"PushEvent"/ { want=1; next }
    want && /"created_at":/ {
        if (match($0, /[0-9]{4}-[0-9]{2}-[0-9]{2}/)) {
            print substr($0, RSTART, 10)
            want=0
        }
    }
')

if [ -z "$DATES" ]; then
    echo "$DEFAULT_DAYS"
    echo 0
    exit 0
fi

LAST_DATE=$(printf '%s\n' "$DATES" | sort -r | head -1)
COUNT=$(printf '%s\n' "$DATES" | wc -l | tr -d ' ')

NOW_TS=$(date +%s)
LAST_TS=$(date -d "$LAST_DATE" +%s 2>/dev/null)

if [ -z "$LAST_TS" ]; then
    echo "$DEFAULT_DAYS"
    echo "$COUNT"
    exit 0
fi

DIFF=$(( (NOW_TS - LAST_TS) / 86400 ))
[ "$DIFF" -lt 0 ] && DIFF=0

echo "$DIFF"
echo "$COUNT"
exit 0
