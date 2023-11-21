PREFIX=/usr/local
CC=gcc
CFLAGS=-O3 -Wall -I../../libs
LDLIBS=-L../../libs/exanic -lexanic -lpthread -lpcap
BIN=./bin

SRC= $(wildcard *.c) $(wildcard ./common/*.c)
OBJ=$(SRC:.c=.o)


all: exareplay

exareplay: $(SRC)
	$(CC) $(CFLAGS) $^ -g -o $(BIN)/$@ $(LDLIBS) 


%:%.c
	$(CC) $(CFLAGS) $^ -g -o $(BIN)/$@ $(LDLIBS)


clean:
	rm -rf $(BIN)/*