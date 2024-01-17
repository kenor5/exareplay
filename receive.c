#include <pthread.h>
#include "exareplay.h"
#include <signal.h>
#include <exanic/fifo_rx.h>

volatile bool finished = false;
void signal_handler(int sig)
{
    finished = true;
}

/* capture sent packets */
void *
thread_capture(void *args)
{
    char device[16] = "exanic0";
    uint32_t port = 1;
    const char *savefile = "test.pcap";
    exanic_t *exanic;
    exanic_rx_t *rx;
    char rx_buf[16384];
    ssize_t rx_size;
    FILE *savefp = NULL;
    exanic_cycles32_t timestamp;
    struct exanic_timespecps ts;
    int pkg_cnt = 0;

    // port = atoi(args);
    // savefile = (char *)args + 4;

    if (savefile != NULL) {
        savefp = fopen(savefile, "w");
        if (!savefp) {
            perror(savefile);
            goto err_open_savefp;
        }
    }else return NULL;

    // write pcap header
    pcap_hdr_t pcap_header = {
        .magic_number = 0xa1b23c4d, /* nanosecond */
        .version_major = 2,
        .version_minor = 4,
        .thiszone = 0,
        .sigfigs = 0,
        .snaplen = 65535,
        .network = 1
    };
    fwrite(&pcap_header, sizeof(pcap_header), 1, savefp);


    exanic = exanic_acquire_handle(device);
    if (exanic == NULL) {
        fprintf(stderr, "Failed to acquire handle for device %s\n", device);
        goto err_acquire_handle;
    }

    rx = exanic_acquire_rx_buffer(exanic, port, 0);
    if (rx == NULL) {
        fprintf(stderr, "Failed to acquire rx buffer for port %u\n", port);
        goto err_acquire_rx_buffer;
    }

    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, signal_handler);
    signal(SIGALRM, signal_handler);
    signal(SIGTERM, signal_handler);
    fprintf(stdout, "Capturing packets at %s port %d\n", device, port);

    // TODO: add finished flag
    while(!finished) {
        rx_size = exanic_receive_frame(rx, rx_buf, sizeof(rx_buf), &timestamp);

        if (rx_size <= 0) continue;

        // get hardware timestamp
        const uint64_t timestamp_ns = exanic_expand_timestamp(exanic, timestamp);
        exanic_cycles_to_timespecps(exanic, timestamp_ns, &ts);

        pcaprec_hdr_t pcaprec_hdr = {
            .ts = {
                .tv_sec = ts.tv_sec,
                .tv_usec = ts.tv_psec / 1000
            },
            .incl_len = rx_size,
            .len = rx_size
        };
        // write pcap record header
        fwrite(&pcaprec_hdr, sizeof(pcaprec_hdr), 1, savefp);
        // write to file
        fwrite(rx_buf, 1, rx_size, savefp);

        pkg_cnt++;
    }

    exanic_release_rx_buffer(rx);
    exanic_release_handle(exanic);
    if (savefile != NULL)
        fclose(savefp);

    fprintf(stdout, "received %d packets\n", pkg_cnt);
    return NULL;

err_acquire_rx_buffer:
    exanic_release_rx_buffer(rx);
err_acquire_handle:
    if (savefile != NULL) {
        fclose(savefp);
    }
err_open_savefp:
    return NULL;
}

/* receive response from test system */
void *
thread_receive_resp(void *args)
{   
    return NULL;
}

int main()
{
    thread_capture(NULL);
    return 0;
}