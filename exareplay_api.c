#include "exareplay_api.h"
#include "common.h"
#include "slot.h"
#include <getopt.h>
#include <stdlib.h>

exareplay_t *
exareplay_init()
{
    exareplay_t *ctx;
    ctx = safe_malloc(sizeof(exareplay_t));
    ctx->opts = safe_malloc(sizeof(exareplay_opt_t));

    /* send one file */
    ctx->opts->dualmode = false;

    /* disable skip any interval*/
    ctx->opts->skip_interval = false;

    ctx->opts->skip_interval_size = 0x7fffffff;

    ctx->opts->input_name = NULL;
    return ctx;
}

void
exareplay_free(exareplay_t *ctx)
{
    safe_free(ctx->opts);
    safe_free(ctx);
}

void
device_init(exareplay_t *ctx)
{
    device_t *device = safe_malloc(sizeof(device_t));
    if ((device->exanic = exanic_acquire_handle(ctx->opts->device)) == NULL) {
        fprintf(stderr, "%s: %s\n", ctx->opts->device, exanic_get_last_error());
        exit(-1);
    }

    /* Reserve some space in the TX buffer. */
    device->tx = exanic_acquire_tx_buffer(device->exanic, ctx->opts->port, TX_SLOT_NUM * TX_SLOT_SIZE);
    if (!device->tx) {
        fprintf(stderr, "exanic_acquire_tx: %s\n", exanic_get_last_error());
        exit(-1);
    }
    ctx->device = device;
}

void
device_close(exareplay_t *ctx)
{
    device_t *device = ctx->device;
    exanic_release_tx_buffer(device->tx);
    exanic_release_handle(device->exanic);
}

int
exareplay_parse_args(exareplay_t *ctx, int argc, char *argv[])
{
    int c;
    if (argc < 2)
        goto usage_err;
    while ((c = getopt(argc, argv, "r:i:ds:h")) != -1) {
        switch (c) {
        case 'r':
            ctx->opts->input_name = optarg;
            break;
        case 'i':
            ctx->opts->device = strtok(optarg, ":");
            ctx->opts->port = atoi(strtok(NULL, ":"));
            break;
        case 'd':
            ctx->opts->dualmode = true;
            break;
        case 's':
            ctx->opts->skip_interval = true;
            ctx->opts->skip_interval_size = atoi(optarg);
            break;
        default:
            goto usage_err;
        }
    }
    return 0;

usage_err:
    fprintf(stderr, "Usage: %s \n", argv[0]);
    fprintf(stderr, "  -r <input_file>    input file name\n");
    fprintf(stderr, "  -i <device:port>   NIC device and port, for example 'exanic0:0'\n");
    fprintf(stderr, "  -d                 replay two file at the same time\n");
    fprintf(stderr, "  -s <interval>      skip large time interval in pcap\n");
    exit(-1);
}

void
pcap_info_init(exareplay_t *ctx)
{
    uint32_t pcap_size;

    /* get pcap size */
    pcap_size = get_pcap_size(ctx->opts->input_name);

    /* malloc pcap memory */
    ctx->pcap_info = safe_malloc(sizeof(pcap_info_t));
    ctx->pcap_info->size = pcap_size;
    ctx->pcap_info->len = safe_malloc(sizeof(u_int32_t) * pcap_size);
    ctx->pcap_info->time_interval = safe_malloc(sizeof(u_int64_t) * pcap_size);
    ctx->pcap_info->data = safe_malloc(sizeof(char *) * pcap_size);
    for (int i = 0; i < pcap_size; i++) {
        ctx->pcap_info->data[i] = safe_malloc(sizeof(char) * MTU);
    }
}

void
pcap_info_free(exareplay_t *ctx)
{
    for (int i = 0; i < ctx->pcap_info->size; i++) {
        safe_free(ctx->pcap_info->data[i]);
    }
    safe_free(ctx->pcap_info->data);
    safe_free(ctx->pcap_info->len);
    safe_free(ctx->pcap_info->time_interval);
    safe_free(ctx->pcap_info);
}

uint32_t
get_pcap_size(char *filename)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *cap;
    struct pcap_pkthdr *pkt_header = safe_malloc(sizeof(struct pcap_pkthdr));
    uint32_t size = 0;

    /* use nanosecond timestamp */
    cap = pcap_open_offline_with_tstamp_precision(filename, PCAP_TSTAMP_PRECISION_NANO, errbuf);
    if (cap == NULL) {
        fprintf(stderr, "error while opening pcap file\n");
        return 0;
    }

    while (pcap_next(cap, pkt_header) != NULL) {
        size++;
    }
    pcap_close(cap);
    return size;
}

void
exareplay_replay(exareplay_t *ctx)
{
    exanic_tx_t *tx = ctx->device->tx;

    uint32_t pcap_num = ctx->pcap_info->size;
    uint64_t *time_interval = ctx->pcap_info->time_interval;
    uint32_t *len = ctx->pcap_info->len;
    // char **pcap_data = ctx->pcap_info->data;

    register int i;
    register uint64_t last_time = 0;
    register uint64_t cur_time_interval = len[0];

    for (i = 0; i < pcap_num;) {
        if (rdtsc() - last_time >= cur_time_interval) {
            trigger_slot_send(tx, i % TX_SLOT_NUM);
            last_time = rdtsc();
            if (i > update_slot_until && i + cur_update < pcap_num) {
                fill_slot(ctx, i + cur_update, (i + cur_update) % TX_SLOT_NUM);
                flush_wc_buffers(tx);
            }
            cur_time_interval = time_interval[++i];
        }
    }
}