PREFIX=/usr/local
CC=gcc
CFLAGS=-O3 -Wall -I../../../libs
LDLIBS=-L../../../libs/exanic -lexanic -lpthread -lpcap
BIN=./bin

all: pcap_filter pcap_generator

pcap_filter: pcap_filter.c pcap_dump.c
	$(CC) $(CFLAGS) $^ -o $(BIN)/$@ $(LDLIBS) 

pcap_generator: pcap_generator.c pcap_dump.c
	$(CC) $(CFLAGS) $^ -g -o $(BIN)/$@ $(LDLIBS)

%:%.c
	$(CC) $(CFLAGS) $^ -g -o $(BIN)/$@ $(LDLIBS)


clean:
	rm -rf $(BIN)/*