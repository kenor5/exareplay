#include "send_packets.h"
#include <sys/types.h>

void load_pcap(exareplay_t *ctx) {
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr *pkt_header = safe_malloc(sizeof(struct pcap_pkthdr));
    pcap_t *cap;
    int idx;
    long int pre_time;

    /* load pcap data */
    cap = pcap_open_offline_with_tstamp_precision(ctx->opts->input_name, PCAP_TSTAMP_PRECISION_NANO, errbuf);
    if (cap == NULL) {
        fprintf(stderr, "error while opening pcap file\n");
        return;
    }

    idx = 0;
    pre_time = 0;
    while (pcap_next(cap, pkt_header) != NULL) {
        memcpy(ctx->pcap_info->data[idx], pkt_header, pkt_header->len);
        ctx->pcap_info->len[idx] = pkt_header->len;
        ctx->pcap_info->time_interval[idx] = SEC_TO_NANOSEC(pkt_header->ts.tv_sec) + (long)pkt_header->ts.tv_usec - pre_time;
        ctx->pcap_info->time_interval[idx] = (long)max(ctx->pcap_info->time_interval[idx] - time_delta, 0);
        ctx->pcap_info->time_interval[idx] = (long)(((ctx->pcap_info->time_interval[idx] > burst_interval_min && ctx->pcap_info->time_interval[idx] < burst_interval_max)
                                                     ? ctx->pcap_info->time_interval[idx] - time_delta_burst_start
                                                     : ctx->pcap_info->time_interval[idx]) *
                                                    ticks_per_nano);

        pre_time = (long)pkt_header->ts.tv_sec * 1e9 + (long)pkt_header->ts.tv_usec;
        idx++;
    }
    pcap_close(cap);
    LOG("pcap loaded\n");
}

