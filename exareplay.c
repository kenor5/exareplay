#include "exareplay.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

exareplay_t *ctx;

pthread_t tid[THREAD_NUM];

uint32_t pcap_num;

uint64_t get_time_interval(uint64_t cur_time, uint64_t pre_time) {
    uint64_t interval = cur_time - pre_time;
    interval = interval - time_delta > 0x7fffffff ? 0 : (interval - time_delta);  /* prevent overflow */
    interval =
                (uint64_t)(((interval > burst_interval_min && interval < burst_interval_max)
                                ? interval - time_delta_burst_start
                                : interval) *
                       ticks_per_nano);
    return interval;
}

void *
thread_disk2mem(void *args)
{
    const char *filename = ctx->opts->input_name;
    
    /* open pcapfile*/
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open %s failed\n", filename);
        exit(-1);
    }
    
    pcap_file_t *cap = mypcap_open(filename);
    int idx = 0;
    uint64_t pre_time = 0;
    pcaprec_hdr_t *pkt_header = safe_malloc(sizeof(pcaprec_hdr_t));
    pcap_info_t *pcap_info;
    int res;
    int size = ctx->pcap_mem_size;

    for (int i = 0; i < size; ++i) {
        pcap_info = &ctx->pcap_info[i];
        res = mypcap_readnext(cap, pkt_header, pcap_info->data);
        if (res == ERROR) {
            fprintf(stderr, "read pcap file error\n");
            exit(-1);
        }else if (res == EOF_REACHED)
            break;

        if (ctx->opts->remove_fcs)
                pcap_info->len = pkt_header->len - 4;
        else
            pcap_info->len = pkt_header->len;
        
        pcap_info->time_interval = get_time_interval(TIMESPEC_TO_NANOSEC(pkt_header->ts), pre_time);

        pre_time = TIMESPEC_TO_NANOSEC(pkt_header->ts);
        idx++;
    }
    ctx->pcap_mem_loaded = true;

    while (true) {
        if (ctx->pcap_mem_use_ptr > ctx->pcap_mem_store_ptr + 10) {
            pcap_info = &ctx->pcap_info[(ctx->pcap_mem_store_ptr++)%size];
            res = mypcap_readnext(cap, pkt_header, pcap_info->data);
            if (res == ERROR) {
                fprintf(stderr, "read pcap file error\n");
                exit(-1);
            }else if (res == EOF_REACHED)
                break;

            if (ctx->opts->remove_fcs)
                pcap_info->len = pkt_header->len - 4;
            else
                pcap_info->len = pkt_header->len;
            
            pcap_info->time_interval = get_time_interval(TIMESPEC_TO_NANOSEC(pkt_header->ts), pre_time);

            pre_time = TIMESPEC_TO_NANOSEC(pkt_header->ts);
            idx++;

        }
    }
    mypcap_close(cap);

    return NULL;
}

void *
thread_NICsend(void *args)
{
    pcap_info_t *pcap_info;
    slot_t *slot_info = &ctx->slot_info;
    uint64_t *slot_time_interval = safe_malloc(sizeof(uint64_t) * TX_SLOT_NUM);

    register uint64_t last_time = 0;
    register uint64_t cur_time_interval;
    register uint32_t i = 0;

    LOG("thread_NICsend started\n");

    pcap_info = ctx->pcap_info;
    cur_time_interval = 0;

    /* wait until memory loaded */
    while (!ctx->pcap_mem_loaded)
        ;
    
    /* load slot */
    for (i = 0; i < TX_SLOT_NUM-2; ++i) {
        slot_time_interval[i] = fill_slot(ctx);
    }
    flush_wc_buffers(ctx->device->tx);

    i = 0;
    while (i < pcap_num) {

        while (rdtsc() - last_time < cur_time_interval)
            ;

        last_time = rdtsc();
        trigger_slot_send(ctx, slot_info->head % TX_SLOT_NUM);
        slot_info->head++;

        cur_time_interval = slot_time_interval[slot_info->head % TX_SLOT_NUM];
        slot_time_interval[(slot_info->tail-1) % TX_SLOT_NUM]= fill_slot(ctx);

        ++i;
    }
    return NULL;
}

void
start_threads()
{
    pthread_create(&tid[0], NULL, thread_disk2mem, NULL);
    frank_pthread_single_cpu_affinity_set(0, tid[0]);

    pthread_create(&tid[1], NULL, thread_NICsend, NULL);
    frank_pthread_single_cpu_affinity_set(1, tid[1]);
}

void
join_threads()
{
    for (int i = 0; i < THREAD_NUM; ++i)
        pthread_join(tid[i], NULL);
}

int
main(int argc, char *argv[])
{
    int rcode;

    fflush(NULL);

    ctx = exareplay_init();

    rcode = exareplay_parse_args(ctx, argc, argv);
    if (rcode < 0) {
        fprintf(stderr, "parse args error\n");
        return -1;
    }

    device_init(ctx);

    pcap_info_init(ctx);

    pcap_num = get_pcap_size(ctx->opts->input_name);

    if (pcap_num > ctx->opts->pcap_cnt)
        pcap_num = ctx->opts->pcap_cnt;

    start_threads();

    join_threads();

    pcap_info_free(ctx);

    device_close(ctx);

    exareplay_free(ctx);

    return 0;
}