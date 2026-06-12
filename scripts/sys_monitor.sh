#!/bin/sh
# scripts/sys_monitor.sh
# top 명령어를 1회(b n1) 실행한 후 Cpu(s) 라인을 찾아 유저 사용량(us)과 시스템 사용량(sy)을 더해 정수로 출력
top -bn1 | grep "Cpu(s)" | awk '{print $2 + $4}' | cut -d. -f1