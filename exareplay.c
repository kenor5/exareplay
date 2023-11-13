#include "exareplay.h"
#include <pthread.h>

exareplay_t *ctx;

pthread_t tid[THREAD_NUM];

uint32_t pcap_num;

void *thread_disk2memory(void *args) {
    const char *filename = ctx->opts->input_name;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct pcap_pkthdr *pkt_header = safe_malloc(sizeof(struct pcap_pkthdr));
    pcap_t *cap;
    int idx;
    long int pre_time;

    LOG("thread_disk2memory started\n");
    
    /* load pcap data */
    cap = pcap_open_offline_with_tstamp_precision(filename, PCAP_TSTAMP_PRECISION_NANO, errbuf);
    if (cap == NULL) {
        fprintf(stderr, "error while opening pcap file\n");
        exit(-1);
    }
    idx = 0;
    pre_time = 0;
    char *buf_ptr;
    while ((buf_ptr = pcap_next(cap, pkt_header)) != NULL) {

        /* wait for ringbuffer to be not full */
        while (!ringbuffer_tofill(ctx->pcap_info));

        LOG("disk2mem %d\n", idx);
        pcap_info_t *pcap_info = safe_malloc(sizeof(pcap_info_t));
        pcap_info->len = pkt_header->len;
        pcap_info->time_interval = TIMESPEC_TO_NANOSEC(pkt_header->ts) - pre_time;
        pcap_info->time_interval = (long)max(pcap_info->time_interval - time_delta, 0);
        pcap_info->time_interval =
                (long)(((pcap_info->time_interval > burst_interval_min &&
                         pcap_info->time_interval < burst_interval_max)
                                ? pcap_info->time_interval - time_delta_burst_start
                                : pcap_info->time_interval) *
                       ticks_per_nano);
        memcpy(pcap_info->data, buf_ptr, pkt_header->len);
        ringbuffer_push(ctx->pcap_info, pcap_info);
        LOG("th 1 push , ringbuffersize %ld\n", ringbuffer_size(ctx->pcap_info));
        pre_time = TIMESPEC_TO_NANOSEC(pkt_header->ts);
        idx++;
    }
    pcap_close(cap);
    ctx->load_complete = true;
    return NULL;
}

void *thread_memory2NIC(void *args) {
    slot_t *slot_info = &ctx->slot_info;
    ringbuffer_t *pcap_info = ctx->pcap_info;
    register uint32_t i = 0;

    LOG("thread_memory2NIC started\n");

    while (ringbuffer_tofill(pcap_info) && !ctx->load_complete) {
        /* wait for ringbuffer to be filled */
    }

    while (i++ < pcap_num) {
        while (slot_info->size >= slot_info->cap - 2  || ringbuffer_useup(pcap_info)) {
            /* wait for slot to be not full */
        }

        LOG("mem2NIC %d\n", slot_info->tail);
        fill_slot(ctx);
        flush_wc_buffers(ctx->device->tx);
    }
    return NULL;
}

void *thread_NICsend(void *args) {
    // exanic_tx_t *tx = ctx->device->tx;
    slot_t *slot_info = &ctx->slot_info;

    ringbuffer_t *pcap_info = ctx->pcap_info;
    pcap_info_t *pcap_info_data = ringbuffer_front(pcap_info);

    register uint64_t last_time = 0;
    register uint64_t cur_time_interval = pcap_info_data->time_interval;
    register uint32_t i = 0;

    LOG("thread_NICsend started\n");

    /* sleep until slot is filled */
    while (slot_info->size < slot_info->cap - 2) ;
    while (i < pcap_num) {

        if (rdtsc() - last_time >= cur_time_interval){
            if (slot_info->size <= 0) {
                LOG("slot_info->size <= 0\n");
                exit(-1);
            }
            last_time = rdtsc();
            trigger_slot_send(ctx, slot_info->head);
            
            ringbuffer_pop(pcap_info);
            cur_time_interval = ((pcap_info_t *)ringbuffer_front(pcap_info))->time_interval;
            
            // LOG("th 3 pop, ringbuffersize %ld, interval %lld\n", ringbuffer_size(pcap_info), cur_time_interval);
            ++i;
        }
    }
    return NULL;
}

void start_threads() {
    pthread_create(&tid[0], NULL, thread_disk2memory, NULL);
    frank_pthread_single_cpu_affinity_set(0, tid[0]);

    pthread_create(&tid[1], NULL, thread_memory2NIC, NULL);
    frank_pthread_single_cpu_affinity_set(1, tid[1]);
    
    pthread_create(&tid[2], NULL, thread_NICsend, NULL);
    frank_pthread_single_cpu_affinity_set(2, tid[2]);
}

void join_threads() {
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

    slot_init(ctx);

    pcap_num = get_pcap_size(ctx->opts->input_name);

    start_threads();

    join_threads();

    pcap_info_free(ctx);

    device_close(ctx);

    exareplay_free(ctx);

    return 0;
}