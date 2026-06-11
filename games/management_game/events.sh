list_of_events=()


trigger_random_event () {
    #local eventIndex=$(( RANDOM % ${#list_of_events[@]} ))
    #local eventFunction=${list_of_events[$eventIndex]}
    #$eventFunction

    local evenIndex=$(( RANDOM % 10 + 1 ))

    case $evenIndex in
        1)
            quick_message "A storm has damaged your coal mines, reducing their production by 50% for the next 3 turns."
            ;;
        2)
            quick_message "A new technology has been discovered, reducing the cost of iron ore by 20% for the next 5 turns."
            ;;
        3)
            quick_message "A labor strike has occurred at your iron plant, halting production for the next turn."
            ;;
        4)
            quick_message "A competitor has entered the market, doubling the price of iron ores for this turn!"
            ironOrePriceModifier=2
            ;;
        5)
            quick_message "A government subsidy has been announced, providing a one-time bonus of 50 money."
            money=$((money + 50))
            ;;
        6)
            quick_message "An environmental regulation has been implemented, increasing the cost of coal by 10% for the next 6 turns."
            ;;
        7)
            quick_message "A new contract has been offered, providing a lucrative opportunity to sell iron at a premium price for the next 3 turns."
            
            read -p "How many iron do you want to sell? " quantity
            
            read -p "Do you want to accept this contract? (y/n) " choice
            
            if [[ $choice == "y" ]]; then
                itemPrice=$(( effectiveIronCost * 2 ))
                contracts_list+=("sell|iron|${quantity}|${effectiveIronCost}|3")
            fi
            ;;
        8)
            quick_message "A natural disaster has struck, damaging your steel plant and reducing its production by 30% for the next 4 turns."
            ;;
        9)
            quick_message "A market boom has occurred, increasing demand for all products and raising prices by 20% for the next 5 turns."
            ;;
        10)
            quick_message "A supply chain disruption has occurred, increasing the cost of all raw materials by 25% for the next 3 turns."
            ;;
    esac

}
event_occurs () {
    local eventChance=$(( RANDOM % 100 + 1 ))
    if [ $eventChance -le 20 ]; then
        trigger_random_event
    fi
}