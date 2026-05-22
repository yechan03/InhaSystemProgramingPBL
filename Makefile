CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -O2 -Isrc
SRC      = src/lobby.c src/account.c
BIN      = bin/lobby

.PHONY: all run clean init

all: $(BIN)

$(BIN): $(SRC)
	@mkdir -p bin data
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

init:
	@sh scripts/init.sh

run: $(BIN)
	@mkdir -p data
	./$(BIN)

clean:
	rm -rf bin
