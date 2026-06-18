#!/bin/bash


USERNAME="$1"
GITHUB="$2"
PIPE_FD="$3"


#GAME STARTS HERE

# 게임 모듈들은 management_game/ 디렉토리에 위치 (이 스크립트만 로비 규칙에 맞춰 games/game4.sh)
MODULE_DIR="$(dirname "$0")/management_game"

source "$MODULE_DIR/change_turn.sh"
source "$MODULE_DIR/input.sh"
source "$MODULE_DIR/display.sh"
source "$MODULE_DIR/resources.sh"
source "$MODULE_DIR/utils.sh"
source "$MODULE_DIR/contracts_management.sh"
source "$MODULE_DIR/market.sh"
source "$MODULE_DIR/events.sh"

clear

#Initialize game state
GAMESTATE="TURN"

money=20
coal=10
iron_ore=20
iron=0
steel=0

COAL_COST=2
IRON_ORE_COST=5
IRON_COST=10
STEEL_COST=30
todayCoalCost=$COAL_COST
todayIronOreCost=$IRON_ORE_COST
todayIronCost=$IRON_COST
todaySteelCost=$STEEL_COST
effectiveCoalCost=$todayCoalCost
effectiveIronOreCost=$todayIronOreCost
effectiveIronCost=$todayIronCost
effectiveSteelCost=$todaySteelCost

coalPriceModifier=1
ironOrePriceModifier=1
ironPriceModifier=1
steelPriceModifier=1

coalMines=0
ironMines=0
ironPlants=0
steelPlants=0

COAL_MINE_COST=10
COAL_MINE_PRODUCTION=25

IRON_MINE_COST=20
IRON_MINE_PRODUCTION=15

IRON_PLANT_COST=40
IRON_PLANT_PRODUCTION=8
IRON_PLANT_COAL_COST=10
IRON_PLANT_IRON_ORE_COST=10

STEEL_PLANT_COST=100
STEEL_PLANT_PRODUCTION=5
STEEL_PLANT_COAL_COST=15
STEEL_PLANT_IRON_COST=10

contracts_list=()

write_int() {
    local value=$1
    local b0=$(( value & 0xFF ))
    local b1=$(( (value >> 8) & 0xFF ))
    local b2=$(( (value >> 16) & 0xFF ))
    local b3=$(( (value >> 24) & 0xFF ))
    printf '%b' "\\x$(printf '%02x' "$b0")\\x$(printf '%02x' "$b1")\\x$(printf '%02x' "$b2")\\x$(printf '%02x' "$b3")" >&"$PIPE_FD"
}

report_score() {
    FINAL_SCORE=$((money + (coal * 2) + (iron_ore * 3) + (iron * 5) + (steel * 10)))
    EXIT_SCORE=$(( FINAL_SCORE / 100 ))
    if [ "$EXIT_SCORE" -gt 255 ]; then
        EXIT_SCORE=255
    fi

    if [ -n "$PIPE_FD" ]; then
        write_int "$EXIT_SCORE"
    fi

    exit 0
}

trap report_score SIGINT

#MAIN
while true; do
    clear
    display_game
    input_management
    if [ "$GAMESTATE" == "QUIT" ]; then
        break
    fi
    if [ "$GAMESTATE" == "PASSING_TURN" ]; then
        passing_turn
    fi
done

report_score