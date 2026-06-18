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

#Color table
RED='\033[0;31m'
NC='\033[0m'