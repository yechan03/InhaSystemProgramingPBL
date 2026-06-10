min( ) {
    if [ $1 -lt $2 ]; then
        echo $1
    else
        echo $2
    fi
}

quick_message() {
    echo "$1"
    sleep 1
}