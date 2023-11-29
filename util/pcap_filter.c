#include <stdio.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <linux/ethtool.h>
#include <time.h>

#include <pcap.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pcap_dump.h"

#define DOLOG
#ifdef DOLOG
    #define LOG(fmt, args...) \
    printf(fmt, ##args);
#else
    #define LOG(fmt, args...)
#endif

#define TIME_DELTA 5
#define SECPERDAY 60*60*24

uint32_t pcap_num = 0;
uint32_t drop_num = 0;

/**
 * @param pkthdr packet header
 * @param packet data payload
 * @param udp_only if only udp packet permitted
 * @param non_trading_only if only filtering packet in non_trading hours
 * @param interval time interval larger than 'interval' will be squeezed
 * @param size_limit packet with size less than 'size_limit' will be drop out. default 70 Byte
 * @param fd output file descriptor
 * 
*/
void packetHandler(const struct pcap_pkthdr* pkthdr, 
					const u_char* packet, 
					bool udp_only, 
					bool non_trading_only, 
					int interval, 
					int size_limit,
					int fd);

/*
 *  return if it is packet at 10:30 or 13:30
 *  TODO: cannot determine exact time according to sec
 */
bool if_in_middle(uint32_t sec);

int main(int argc, char* argv[]) 
{

	int c;
	const char* input_file = NULL, *output_file = NULL;
	int interval = 15;
	int size_limit = 70;
	int num_limit = 2147483647;
	int offset = 0;
	bool udp_only = true;
	bool skip_non_trading_only = true;
	char errbuf[PCAP_ERRBUF_SIZE];

	if (argc < 3)
		goto usage_error;
	while ((c = getopt(argc, argv, "i:w:ulc:hs:n:o:d")) != -1)
	    {
		switch(c)
		{
			case 'i':
				input_file = optarg;
				break;
			case 'w':
				output_file = optarg;
				break;
			case 'u':
				udp_only = false;
				break;
			case 'l':
				skip_non_trading_only = false;
				break;
			case 'c':
				interval = atoi(optarg);
				break;
			case 's':
				size_limit = atoi(optarg);
				break;
			case 'd':
				interval = SECPERDAY;
				break;
			case 'n':
				num_limit = atoi(optarg);
				break;
			case 'o':
				offset = atoi(optarg);
				break;
			default:
				goto usage_error;
		}

	    }
	LOG("|----------------- args ---------------|\n");
	LOG("  input file:                 %s\n", input_file);
	LOG("  output file:                %s\n", output_file);
	LOG("  min interval:               %d minute(s)\n", interval);
	LOG("  min size:                   %d\n", size_limit);
	LOG("  udp only:                   %s\n", udp_only?"True":"False");
	LOG("  skip non-trading only:      %s\n", skip_non_trading_only?"True":"False");
	LOG("  disable filter by interval  %s\n", size_limit==SECPERDAY?"True":"False");
	LOG("  max pcap num to save:       %d\n", num_limit);
	LOG("  ignore leading o nums:      %d\n\n", offset);

	int fd = pd_open(output_file);
	if (!fd) {
		fprintf(stderr, "err while open output file\n");
		return -1;
	}

	pcap_t *cap;
	struct pcap_pkthdr pkthdr;
	
	cap = pcap_open_offline_with_tstamp_precision(input_file, PCAP_TSTAMP_PRECISION_NANO, errbuf);

	const u_char *data = NULL;

	LOG("|----------------- intervals ---------------|\n");

	while (offset--)
		(void)pcap_next(cap, &pkthdr);
	while ((data = pcap_next(cap, &pkthdr))) {
		if (pcap_num - drop_num >= num_limit) {
			break;
		}
		packetHandler(&pkthdr, 
						data, 
						udp_only, 
						skip_non_trading_only, 
						interval, 
						size_limit,
						fd);
	}

	pd_close(fd);

	LOG("\n|----------------- res ---------------|\n");
	LOG("  total num:             %d\n", pcap_num);
	LOG("  drop num:              %d\n", drop_num);

	return 0;

usage_error:
	fprintf (stderr, "Usage: %s -i input.cap -w output.cap \n", argv[0]);
	fprintf (stderr, "           [-u] [-l] [-d] [-c interval] [-s size] \n");
	fprintf (stderr, "  -u: preserve all packet, include udp and others \n");
	fprintf (stderr, "  -l: skip other interval except non-trading hours \n");
	fprintf (stderr, "  -c: specify min interval, default 15min \n");
	fprintf (stderr, "  -s: filter packet which size is less than size_limit, default 70\n");
	fprintf (stderr, "  -d: disable filter by time interval\n");
	fprintf (stderr, "  -n: specify max pcap num to save\n");
	fprintf (stderr, "  -o: ignore leading o nums of pcap");
}


void packetHandler(const struct pcap_pkthdr* pkthdr, const u_char* packet, bool udp_only, bool non_trading_only, int interval, int size_limit, int fd){
	/* previous valid packet timestamp */
	static struct pd_timeval pre_time = {
		.tv_sec = 0,
		.tv_usec = 0
	};
	/* accumulated time interval */
	static uint32_t acc_timeval = 0;
	static uint32_t cnt_interval = 0;


	/* whether a packet will write to output file*/
	bool writeable = true;

	/* packets with illegal size or transport layer type are specified
	as invalid packet, just return and no need to update pre_time or write file*/
	if (pkthdr->caplen < size_limit) {
		goto drop;
	}

	if (udp_only) {
		 const struct iphdr* ipHeader;
	     ipHeader = (struct iphdr*)(packet + sizeof(struct ether_header));	
		 
		 if (ipHeader->protocol != IPPROTO_UDP) {
		 	goto drop;
		 }
	}

	/* init pre_time */
	if (pcap_num == drop_num) {
		pre_time.tv_sec = (uint32_t)pkthdr->ts.tv_sec;
		pre_time.tv_usec = (uint32_t)pkthdr->ts.tv_usec;
	}

	int cur_interval;
	cur_interval = (uint32_t)pkthdr->ts.tv_sec - pre_time.tv_sec;

	if (cur_interval < 0) cur_interval = 0;
	
	if (cur_interval  > interval * 60 - TIME_DELTA) {
		
		cnt_interval ++;
		
		LOG("  interval id %d -> pcap num %d: interval %d s\n",
			cnt_interval,
			pcap_num+1,
			cur_interval);

		cur_interval = (cur_interval + TIME_DELTA) / 60;

		/* at the start of trading or skip all large interval enabled */
		if (((cur_interval == 15 || cur_interval == 120) && !if_in_middle(pkthdr->ts.tv_sec)) || !non_trading_only) {
			acc_timeval += (cur_interval * 60 - TIME_DELTA);
		}
	}
	goto update_pre_time;

update_pre_time:
	pre_time.tv_sec = (uint32_t)pkthdr->ts.tv_sec;
	pre_time.tv_usec = (uint32_t)pkthdr->ts.tv_usec;
	goto write_pcap;

write_pcap:
	if (writeable) {
		struct pcap_pkthdr tmp_hdr = {
			.ts.tv_sec = pkthdr->ts.tv_sec - acc_timeval,
			.ts.tv_usec = pkthdr->ts.tv_usec
		};
		pd_write(fd, (char*)packet, pkthdr->caplen, tmp_hdr.ts);
	}
	pcap_num ++;
	return;

drop:
	drop_num ++;
	pcap_num ++;
	return;
}

bool if_in_middle(uint32_t sec){
	return false;
}
