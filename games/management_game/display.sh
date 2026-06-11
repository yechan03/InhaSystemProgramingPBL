display_game( ) {
    echo "This is the management game"
    display_info
    display_menus
}
display_info() {
    echo "money: $money"
    echo "coal: $coal"
    echo "iron ores: $iron_ore"
    echo "iron: $iron"
    echo "steel: $steel"
    echo "coal mines: $coalMines"
    echo "iron mines: $ironMines"
    echo "iron plants: $ironPlants"
    echo "steel plants: $steelPlants"
}
display_menus() {
    case $GAMESTATE in
        "TURN")
            echo "1. buy"
            echo "2. sell"
            echo "3. new contract"
            echo "4. list of contracts"
            echo "5. next turn"
            ;;
        "BUY")
            echo "1. coal mine ($COAL_MINE_COST money)"
            echo "2. iron mine ($IRON_MINE_COST money)"
            echo "3. iron plant ($IRON_PLANT_COST money)"
            echo "4. steel plant ($STEEL_PLANT_COST money)"
            echo "5. coal ($effectiveCoalCost money)"
            echo "6. iron ore ($effectiveIronOreCost money)"
            echo "7. iron ($effectiveIronCost money)"
            echo "8. steel ($effectiveSteelCost money)"
            ;;
        "SELL")
            echo "1. coal ($effectiveCoalCost money)"
            echo "2. iron ore ($effectiveIronOreCost money)"
            echo "3. iron ($effectiveIronCost money)"
            echo "4. steel ($effectiveSteelCost money)"
            echo "5. coal mine ($COAL_MINE_COST money)"
            echo "6. iron mine ($IRON_MINE_COST money)"
            echo "7. iron plant ($IRON_PLANT_COST money)"
            echo "8. steel plant ($STEEL_PLANT_COST money)"
            ;;
        "MAKE_CONTRACT")
            echo "1. coal contract"
            echo "2. iron ore contract"
            echo "3. iron contract"
            echo "4. steel contract"
            ;;
        "LIST_CONTRACTS")
            for contract in "${contracts_list[@]}"; do
                IFS='|' read -r buyOrSell product quantity itemPrice turns <<< "$contract"
                echo "$buyOrSell $quantity $product for $itemPrice money each turn, $turns turns left"
            done
            ;;
    esac
}