buy_build() {
    local item=$1
    case $item in
        "coal mine")
            if [ $money -lt $COAL_MINE_COST ]; then
                quick_message "Not enough money to build a coal mine."
                return
            fi
            (( money -= COAL_MINE_COST ))
            (( coalMines += 1 ))
            ;;
        "iron mine")
            if [ $money -lt $IRON_MINE_COST ]; then
                quick_message "Not enough money to build an iron mine."
                return
            fi
            (( money -= IRON_MINE_COST ))
            (( ironMines += 1 ))
            ;;
        "iron plant")
            if [ $money -lt $IRON_PLANT_COST ]; then
                quick_message "Not enough money to build an iron plant."
                return
            fi
            (( money -= IRON_PLANT_COST ))
            (( ironPlants += 1 ))
            ;;
        "steel plant")
            if [ $money -lt $STEEL_PLANT_COST ]; then
                quick_message "Not enough money to build a steel plant."
                return
            fi
            (( money -= STEEL_PLANT_COST ))
            (( steelPlants += 1 ))
            ;;
        "coal")
            read -p "how many coal do you want to buy? " coal_to_buy
            if [ $money -lt $((effectiveCoalCost * coal_to_buy)) ]; then
                quick_message "Not enough money to buy coal."
                return
            fi
            (( money -= $effectiveCoalCost * $coal_to_buy ))
            (( coal += $coal_to_buy ))
            ;;
        "iron ore")
            read -p "how many iron ores do you want to buy? " iron_ore_to_buy
            if [ $money -lt $((effectiveIronOreCost * iron_ore_to_buy)) ]; then
                quick_message "Not enough money to buy iron ore."
                return
            fi
            (( money -= $effectiveIronOreCost * $iron_ore_to_buy ))
            (( iron_ore += $iron_ore_to_buy ))
            ;;
        "iron")
            read -p "how many iron do you want to buy? " iron_to_buy
            if [ $money -lt $((effectiveIronCost * iron_to_buy)) ]; then
                quick_message "Not enough money to buy iron."
                return
            fi
            (( money -= $effectiveIronCost * $iron_to_buy ))
            (( iron += $iron_to_buy ))
            ;;
        "steel")
            read -p "how many steel do you want to buy? " steel_to_buy
            if [ $money -lt $((effectiveSteelCost * steel_to_buy)) ]; then
                quick_message "Not enough money to buy steel."
                return
            fi
            (( money -= $effectiveSteelCost * $steel_to_buy ))
            (( steel += $steel_to_buy ))
            ;;
        *)
            quick_message "Invalid item."
            ;;
    esac
}
sell_destroy() {
    local item=$1
    local quantity=$2
    case $item in
    "coal")
        if [ $coal -lt $quantity ]; then
            quick_message "Not enough coal to sell."
            return
        fi
        (( money += $effectiveCoalCost * $quantity ))
        (( coal -= $quantity ))

    ;;
    "iron ore")
        if [ $iron_ore -lt $quantity ]; then
            quick_message "Not enough iron ores to sell."
            return
        fi
        (( money += $effectiveIronOreCost * $quantity ))
        (( iron_ore -= $quantity ))
    ;;
    "iron")
        if [ $iron -lt $quantity ]; then
            quick_message "Not enough iron to sell."
            return
        fi
        (( money += $effectiveIronCost * $quantity ))
        (( iron -= $quantity ))
    ;;
    "steel")
        if [ $steel -lt $quantity ]; then
            quick_message "Not enough steel to sell."
            return
        fi
        (( money += $effectiveSteelCost * $quantity ))
        (( steel -= $quantity ))
    ;;
    "coal mine")
        if [ $coalMines -lt $quantity ]; then
            quick_message "Not enough coal mines to destroy."
            return
        fi
        (( money += $COAL_MINE_COST * $quantity ))
        (( coalMines -= $quantity ))
    ;;
    "iron mine")
        if [ $ironMines -lt $quantity ]; then
            quick_message "Not enough iron mines to destroy."
            return
        fi
        (( money += $IRON_MINE_COST * $quantity ))
        (( ironMines -= $quantity ))
    ;;
    "iron plant")
        if [ $ironPlants -lt $quantity ]; then
            quick_message "Not enough iron plants to destroy."
            return
        fi
        (( money += $IRON_PLANT_COST * $quantity ))
        (( ironPlants -= $quantity ))
    ;;
    "steel plant")
        if [ $steelPlants -lt $quantity ]; then
            quick_message "Not enough steel plants to destroy."
            return
        fi
        (( money += $STEEL_PLANT_COST * $quantity ))
        (( steelPlants -= $quantity ))
    ;;
    esac
}