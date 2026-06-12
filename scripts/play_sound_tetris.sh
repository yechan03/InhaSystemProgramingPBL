#!/bin/sh
# scripts/play_sound_tetris.sh

# sound/ 폴더에 효과음 mp3나 wav 파일 추가
# paplay(PulseAudio)나 aplay를 백그라운드(&)로 실행하여 게임 루프 렉을 완벽 차단
if [ "$1" = "lock" ]; then
    aplay sound/game3_lock.wav 2>/dev/null &
elif [ "$1" = "clear" ]; then
    aplay sound/game3_clear.wav 2>/dev/null &
elif [ "$1" = "perfectclear" ] || [ "$1" = "PerfectClear" ]; then
    aplay sound/game3_PerfectClear.wav 2>/dev/null &
elif [ "$1" = "gameover" ]; then
    aplay sound/game3_gameover.wav 2>/dev/null &
fi