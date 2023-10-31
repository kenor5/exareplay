#pragma once

#include "exareplay_api.h"
#include <pcap.h>

void send_packets(tcpreplay_t *ctx, pcap_t *pcap, int idx);
void send_dual_packets(tcpreplay_t *ctx, pcap_t *pcap1, int idx1, pcap_t *pcap2, int idx2);
void *cache_mode(tcpreplay_t *ctx, char *cachedata, COUNTER packet_num);
void preload_pcap_file(tcpreplay_t *ctx, int idx);