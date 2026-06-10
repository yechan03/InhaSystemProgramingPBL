cost_per_turn ( ) {
    local contractCost
    
}

contract_resources () {
    local buyOrSell quantity turns separator
    local productToSell=$1
    separator='|'

    read -p "Do you want to buy (1) or sell (2) $productToSell? " buyOrSell
    if [[ $buyOrSell == "1" ]]; then
        buyOrSell="buy"
    else
        buyOrSell="sell"
    fi
    read -p "How many? " quantity
    read -p "For how many turns? " turns
    while [ $turns -le 1 ]; do
        read -p "Please enter a number of turn bigger or equal to 2." turns
    done
    itemPrice=0
    case $productToSell in
        "coal")
            itemPrice=$todayCoalCost
            ;;
        "iron ore")
            itemPrice=$todayIronOreCost
            ;;
        "iron")
            itemPrice=$todayIronCost
            ;;
        "steel")
            itemPrice=$todaySteelCost
            ;;
        *)
            echo "Unknown product: $productToSell"
            return
            ;;
    esac   
    read -p "According to today's market, the price of $productToSell is $itemPrice, so $(( itemPrice * quantity )) each turn. Shall we proceed? (y/n)" proceed
    if [[ $proceed == "y" ]]; then
        contracts_list+=("${buyOrSell}${separator}${productToSell}${separator}${quantity}${separator}${itemPrice}${separator}${turns}")
    fi
    quick_message "Contract added: ${buyOrSell} ${quantity} ${productToSell} for $(( itemPrice * quantity )) each turn for $turns turns."
}

contract_employee () {
    echo "Do you want to hire or fire an employee?"
    echo "How many?"
}