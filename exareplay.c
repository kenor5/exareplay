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

void *
thread_disk2nic(void *args)
{
    const char *filename = ctx->opts->input_name;
    
    /* open pcapfile*/
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open %s failed\n", filename);
        exit(-1);
    }

    /* get file size */
    struct stat statbuf;
    fstat(fd, &statbuf);
    size_t file_size = statbuf.st_size;

    /* mmap file */
    char *file_ptr = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_ptr == MAP_FAILED) {
        fprintf(stderr, "mmap %s failed\n", filename);
        exit(-1);
    }

    pcap_file_t *cap = mypcap_open_memory(file_ptr, file_size);
    
    int idx = 0;
    uint64_t pre_time = 0;
    char *buf_ptr = NULL;
    pcaprec_hdr_t *pkt_header = NULL;
    pcap_info_t pcap_info;
    // unsigned long long start_time;
    while ((buf_ptr = mypcap_next_memory(&cap, &pkt_header)) != NULL) {
        /* wait for ringbuffer to be not full */
        while (!ringbuffer_tofill(ctx->pcap_info))
            ;
        // start_time = rdtsc();
        if (ctx->opts->remove_fcs)
            pcap_info.len = pkt_header->len - 4;
        else
            pcap_info.len = pkt_header->len;
        
        pcap_info.time_interval = TIMESPEC_TO_NANOSEC(pkt_header->ts) - pre_time;
        pcap_info.time_interval = (uint64_t)max(pcap_info.time_interval - time_delta, 0);
        pcap_info.time_interval =
                (uint64_t)(((pcap_info.time_interval > burst_interval_min && pcap_info.time_interval < burst_interval_max)
                                ? pcap_info.time_interval - time_delta_burst_start
                                : pcap_info.time_interval) *
                       ticks_per_nano);

        ringbuffer_push(ctx->pcap_info, &pcap_info);

        fill_slot(ctx, cap->data_ptr);
        flush_wc_buffers(ctx->device->tx);

        cap->data_ptr += pkt_header->incl_len;
        ringbuffer_pop(ctx->pcap_info);
        pre_time = TIMESPEC_TO_NANOSEC(pkt_header->ts);
        idx++;

        // LOG("%lld\n", rdtsc() - start_time);
    }
    mypcap_close(cap);

    return NULL;
}

void *
thread_NICsend(void *args)
{

    ringbuffer_t *pcap_info;
    pcap_info_t *pcap_info_data;

    register uint64_t last_time = 0;
    register uint64_t cur_time_interval;
    register uint32_t i = 0;

    LOG("thread_NICsend started\n");
    return NULL;
    /* sleep until slot is filled */
    while (ringbuffer_tofill(ctx->pcap_info))
        
        ;
    fprintf(stderr, "wait done\n");
    pcap_info = ctx->pcap_info;
    pcap_info_data = ringbuffer_front(pcap_info);
    cur_time_interval = pcap_info_data->time_interval;
    while (i < pcap_num) {

        if (rdtsc() - last_time >= cur_time_interval) {

            last_time = rdtsc();
            trigger_slot_send(ctx, ringbuffer_get_head_idx(ctx->pcap_info));

            ringbuffer_pop(pcap_info);
            // fprintf(stderr, "ring buffer size %ld, send %d\n", ringbuffer_size(pcap_info), i);
            cur_time_interval = ((pcap_info_t *)ringbuffer_front(pcap_info))->time_interval;

            // LOG("th 3 pop, ringbuffersize %ld, interval %lld\n", ringbuffer_size(pcap_info), cur_time_interval);
            ++i;
        }
    }
    return NULL;
}

void
start_threads()
{
    pthread_create(&tid[0], NULL, thread_disk2nic, NULL);
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