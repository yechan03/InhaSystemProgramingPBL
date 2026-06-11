#!/bin/bash



#GAME STARTS HERE

source "$(dirname "$0")/change_turn.sh"
source "$(dirname "$0")/input.sh"
source "$(dirname "$0")/display.sh"
source "$(dirname "$0")/resources.sh"
source "$(dirname "$0")/utils.sh"
source "$(dirname "$0")/contracts_management.sh"
source "$(dirname "$0")/market.sh"
source "$(dirname "$0")/events.sh"

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

#MAIN
while true; do
    clear
    display_game
    input_management
    if [ "$GAMESTATE" == "PASSING_TURN" ]; then
        passing_turn
    fi
done