input_management() {
    read -p "Enter your choice: " choice
    echo $choice
    echo $GAMESTATE
    case $GAMESTATE in
        "TURN")
            echo "You are in the turn menu"
            case $choice in
                1)
                    echo "You chose to buy"
                    GAMESTATE="BUY"
                    break
                    ;;
                2)
                    echo "You chose to sell"
                    GAMESTATE="SELL"
                    ;;
                3)
                    echo "You chose to establish a new contract"
                    GAMESTATE="MAKE_CONTRACT"
                    ;;
                4)
                    echo "You chose to see the list of contracts"
                    GAMESTATE="LIST_CONTRACTS"
                    ;;
                5)
                    GAMESTATE="PASSING_TURN"
                    ;;
                *)
                    echo "Invalid choice"
                    ;;
            esac
            ;;
        "BUY")
            case $choice in
                1)
                    buy_build "coal mine"
                    ;;
                2)
                    buy_build "iron mine"
                    ;;
                3)
                    buy_build "iron plant"
                    ;;
                4)    
                    buy_build "steel plant"
                    ;;
                5)
                    buy_build "coal"
                    ;;
                6)
                    buy_build "iron ore"
                    ;;
                7)
                    buy_build "iron"
                    ;;
                8)
                    buy_build "steel"
                    ;;
            esac
            GAMESTATE="TURN"

            ;;
        "SELL")
            case $choice in
                1)
                    read -p "how many coal do you want to sell? " coal_to_sell
                    sell_destroy "coal" "$coal_to_sell"
                    ;;
                2)
                    read -p "how many iron ores do you want to sell? " iron_ore_to_sell
                    sell_destroy "iron ore" "$iron_ore_to_sell"
                    ;;
                3)
                    read -p "how many iron do you want to sell? " iron_to_sell
                    sell_destroy "iron" "$iron_to_sell"
                    ;;
                4)
                    read -p "how many steel do you want to sell? " steel_to_sell
                    sell_destroy "steel" "$steel_to_sell"
                    ;;
                5)
                    read -p "how many coal mines do you want to destroy? " coal_mines
                    sell_destroy "coal mine" "$coal_mines"
                    ;;
                6)
                    read -p "how many iron mines do you want to destroy? " iron_mines
                    sell_destroy "iron mine" "$iron_mines"
                    ;;
                7)
                    read -p "how many iron plants do you want to destroy? " iron_plants
                    sell_destroy "iron plant" "$iron_plants"
                    ;;
                8)
                    read -p "how many steel plants do you want to destroy? " steel_plants
                    sell_destroy "steel plant" "$steel_plants"
                    ;;
            esac
            GAMESTATE="TURN"
        ;;
        "MAKE_CONTRACT")
            echo "You are in the make contract menu"
                case $choice in
                    1)
                        echo "You chose to make a coal contract"
                        contract_resources "coal"
                        ;;
                    2)
                        echo "You chose to make an iron ore contract"
                        contract_resources "iron ore"
                        ;;
                    3)
                        echo "You chose to make an iron contract"
                        contract_resources "iron"
                        ;;
                    4)
                        echo "You chose to make a steel contract"
                        contract_resources "steel"
                        ;;
                    *)
                        echo "Invalid choice"
                        ;;
                esac
                
                GAMESTATE="TURN"
        ;;
        "LIST_CONTRACTS")
            echo "You are in the list of contracts menu"
                if [ $choice -eq 1 ]; then
                    GAMESTATE="TURN"
                else
                    echo "Invalid choice"
                fi
            GAMESTATE="TURN"
        ;;
    esac

}