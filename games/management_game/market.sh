market_price_fluctuation () {
    itemPrice=$1
    priceFluctuation=$(( RANDOM % 20 - 10 )) # -10 to 10
    newPrice=$(( itemPrice + priceFluctuation ))
    if [ $newPrice -lt 1 ]; then
        newPrice=1
    fi
    echo $newPrice
}
update_market_prices () {
    todayCoalCost=$(market_price_fluctuation $todayCoalCost)
    todayIronOreCost=$(market_price_fluctuation $todayIronOreCost)
    todayIronCost=$(market_price_fluctuation $todayIronCost)
    todaySteelCost=$(market_price_fluctuation $todaySteelCost)
}