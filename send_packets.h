#pragma once

#include "defines.h"
#include "common.h"
#include "exareplay_api.h"
#include <pcap.h>

void send_packets(exareplay_t *ctx, pcap_t *pcap, int idx);
void send_dual_packets(exareplay_t *ctx, pcap_t *pcap1, int idx1, pcap_t *pcap2, int idx2);

void load_pcap(exareplay_t *ctx);
