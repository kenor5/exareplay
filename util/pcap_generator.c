#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pcap.h>

#include "pcap_dump.h"

#define MAX_LINE_LENGTH 1000

/**
 * 
 * input file format
 *     * total number of pcap , 1 col
       * transport layer type of each line, n col
       * time interval between current and former one, n col
       * packet length of current one, n col
*/

int main(int argc, char *argv[]){

	if (argc < 3) {
		goto usage_err;
	}

	const char *input = NULL, *output = NULL;
	char *token;
	char line[MAX_LINE_LENGTH];
	int pcap_num = MAX_LINE_LENGTH, cnt = 0;
	char type[MAX_LINE_LENGTH];
	int interval[MAX_LINE_LENGTH];
	int pcap_len[MAX_LINE_LENGTH];

	input = argv[1];
	output = argv[2];


	for (int i = 0; i < MAX_LINE_LENGTH; ++i) {
		type[i] = 'u';
		interval[i] = 1;
		pcap_len[i] = 1200;
	}
#ifdef USE_INPUT

	FILE *ifile = fopen(input, "r");
	if (!ifile) {
		goto open_err;
	}

	if (!fgets(line, MAX_LINE_LENGTH, ifile)) goto read_err;
	pcap_num = atoi(line);

	if (pcap_num > MAX_LINE_LENGTH) goto oversized_err;

	if (!fgets(line, MAX_LINE_LENGTH, ifile)) goto read_err;

	token = strtok(line, " ");
	cnt = 0;
	while (token && cnt < pcap_num) {
		type[cnt] = token[0];
		token = strtok(NULL, " ");
		cnt ++;
	}

	if (!fgets(line, MAX_LINE_LENGTH, ifile)) goto read_err;

	token = strtok(line, " ");
	cnt = 0;
	while (token && cnt < pcap_num) {
		interval[cnt] = atoi(token);
		token = strtok(NULL, " ");
		cnt ++;
	}

	if (!fgets(line, MAX_LINE_LENGTH, ifile)) goto read_err;

	token = strtok(line, " ");
	cnt = 0;
	while (token && cnt < pcap_num) {
		pcap_len[cnt] = atoi(token);
		token = strtok(NULL, " ");
		cnt ++;
	}
#endif

	char *data = NULL;
	struct pcap_pkthdr pkthdr;
	int acc_sec = 0;
	
    int fd  = pd_open(output);
	if (!fd) goto open_err;

	cnt = 0;
    while (cnt < pcap_num)
    {
		data = (char *)malloc(pcap_len[cnt]);
		if (!data) {
			fprintf(stderr, "err while malloc memory\n");
		}
		memset(data, 0, pcap_len[cnt]);
		acc_sec += interval[cnt];
		// fake MAC address
		*(long *)(data) = 0x4a303a886234;
		*(long *)(data+6) = 0x08bfb8728115;

		// IPV4 and header len
		data[12] = 0x08;
		data[13] = 0x00;
		data[14] = 0x45;

		// fake IP address
	    *(int*)(data+26) = 0x9466010a;
		*(int*)(data+30) = 0xfaffffef;

		if (type[cnt] == 'u'){
			// UDP
			data[23] = 0x11;
			// header len
			data[39] = 0x08;
		}
		else {
			// TCP
			data[23] = 0x06;
			// header len
			data[46] = 0x50;
			data[49] = 0x10;
		}

		pkthdr.ts.tv_usec = cnt*1500;
		pkthdr.ts.tv_sec = 0;

		if (fd) pd_write(fd, (char*)data, pcap_len[cnt], pkthdr.ts);
		printf("%d\n", pcap_len[cnt]);
		cnt ++;

		free(data);
		data = NULL;
    }

    pd_close(fd);

	return 0;

usage_err:
	fprintf(stderr, "usage error. %s [input] [output]\n", argv[0]);
	return -1;

open_err:
	fprintf(stderr, "err while open input file\n");
	return -1;

read_err:
	fprintf(stderr, "read file error\n");
	// fclose(ifile);
	return -1;

oversized_err:
	fprintf(stderr, "input pcap num too large\n");
	// fclose(ifile);
	return -1;
}
