
buy_contract () {
    local product=$1
    local quantity=$2
    local itemPrice=$3
    case $product in
        "coal")
            (( coal+=$quantity ))
            ;;
        "iron ore")
            (( iron_ore+=$quantity ))
            ;;
        "iron")
            (( iron+=$quantity ))
            ;;
        "steel")
            (( steel+=$quantity ))
            ;;
        esac
    turnCost=$(( quantity * itemPrice ))
    (( money -= turnCost ))

}
sell_contract () {
    local product=$1
    local quantity=$2
    local itemPrice=$3
    case $product in
        "coal")
            if [ $coal -ge $quantity ]; then
                (( coal -= quantity ))
            else
                echo "Not enough $product to sell."
                return 1
            fi
            ;;
        "iron ore")
            if [ $iron_ore -ge $quantity ]; then
                (( iron_ore -= quantity ))
            else
                echo "Not enough $product to sell."
                return 1
            fi
            ;;
        "iron")
            if [ $iron -ge $quantity ]; then
                (( iron -= quantity ))
            else
                echo "Not enough $product to sell."
                return 1
            fi
            ;;
        "steel")
            if [ $steel -ge $quantity ]; then
                (( steel -= quantity ))
            else
                echo "Not enough $product to sell."
                return 1
            fi
            ;;
        esac
        turnCost=$(( quantity * itemPrice ))
        (( money += turnCost ))
    return 0
}
contracts_execution() {
    new_contracts_list=()

    
    for contracts in "${contracts_list[@]}"
    do
        local buyOrSell product quantity itemPrice turns
        IFS='|' read -r buyOrSell product quantity itemPrice turns <<< "$contracts"
        if [ "$buyOrSell" == "buy" ]; then
            buy_contract $product $quantity $itemPrice
        else
            if ! sell_contract $product $quantity $itemPrice; then
                echo "Failed to execute contract: not enough resources to sell."
                continue
            fi
        fi
        (( turns-- ))
        if (( turns > 0 )); then
            new_contracts_list+=("${buyOrSell}|${product}|${quantity}|${itemPrice}|${turns}")
        fi
    done
    contracts_list=("${new_contracts_list[@]}")
}


# Occurs at the end of the turn, calculates the new resources produced and consumed by the plants,
# and updates the resources count accordingly
resource_production() {
    newCoal=$(( coalMines * COAL_MINE_PRODUCTION ))
    (( coal += newCoal ))
    newIronOre=$(( ironMines * IRON_MINE_PRODUCTION ))
    (( iron_ore += newIronOre ))

    # If we don't have enough resources, somes plants cannot be fueled
    how_many_iron_plants_can_be_fueled_by_coal=$(( coal / IRON_PLANT_COAL_COST ))
    how_many_iron_plants_can_be_fueled_by_iron_ore=$(( iron_ore / IRON_PLANT_IRON_ORE_COST ))
    how_many_iron_plants_can_be_fueled=$( min $how_many_iron_plants_can_be_fueled_by_coal $how_many_iron_plants_can_be_fueled_by_iron_ore )
    newIron=0
    coalConsumed=0
    ironOresConsumed=0
    ironConsumed=0

    if [ $how_many_iron_plants_can_be_fueled -lt $ironPlants ]; then
        echo "Not enough coal or iron ores to fuel all iron plants. Only $how_many_iron_plants_can_be_fueled can be fueled."
        newIron=$(( how_many_iron_plants_can_be_fueled * IRON_PLANT_PRODUCTION ))
        (( coalConsumed += how_many_iron_plants_can_be_fueled * IRON_PLANT_COAL_COST ))
        (( ironOresConsumed += how_many_iron_plants_can_be_fueled * IRON_PLANT_IRON_ORE_COST ))
    else
        newIron=$(( ironPlants * IRON_PLANT_PRODUCTION ))
        (( coalConsumed += ironPlants * IRON_PLANT_COAL_COST ))
        (( ironOresConsumed += ironPlants * IRON_PLANT_IRON_ORE_COST ))
    fi

    how_many_steel_plants_can_be_fueled_by_coal=$(( coal / STEEL_PLANT_COAL_COST ))
    how_many_steel_plants_can_be_fueled_by_iron=$(( iron / STEEL_PLANT_IRON_COST ))
    how_many_steel_plants_can_be_fueled=$( min $how_many_steel_plants_can_be_fueled_by_coal $how_many_steel_plants_can_be_fueled_by_iron )
    newSteel=0

    if [ $how_many_steel_plants_can_be_fueled -lt $steelPlants ]; then
        echo "Not enough coal or iron to fuel all steel plants. Only $how_many_steel_plants_can_be_fueled can be fueled."
        newSteel=$(( how_many_steel_plants_can_be_fueled * STEEL_PLANT_PRODUCTION ))
        (( coalConsumed += how_many_steel_plants_can_be_fueled * STEEL_PLANT_COAL_COST ))
        (( ironConsumed += how_many_steel_plants_can_be_fueled * STEEL_PLANT_IRON_COST ))
    else
        newSteel=$(( steelPlants * STEEL_PLANT_PRODUCTION ))
        (( coalConsumed += steelPlants * STEEL_PLANT_COAL_COST ))
        (( ironConsumed += steelPlants * STEEL_PLANT_IRON_COST ))
    fi
    
    (( iron += newIron ))
    (( steel += newSteel ))
    (( coal -= coalConsumed ))
    (( iron_ore -= ironOresConsumed ))
    (( iron -= ironConsumed ))
}



passing_turn() {
    clear
    echo "The night fades and a new sun rises."
    update_market_prices
    resource_production
    contracts_execution
    echo "you got $newCoal coal"
    echo "you consumed $coalConsumed coal"
    echo "you have $coal coal now"
    echo "you consumed $ironOresConsumed iron ores"
    echo "you got $newIronOre iron ores"
    echo "you have $iron_ore iron ores now"
    echo "you consumed $ironConsumed iron"
    echo "you got $newIron iron"
    echo "you have $iron iron now"
    echo "you got $newSteel steel"
    echo "you have $steel steel now"

    read -p "Press key to continue.. " -n1 -s

    GAMESTATE="TURN"
}