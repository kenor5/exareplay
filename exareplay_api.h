#pragma once

#include "defines.h"
#include <exanic/exanic.h>
#include <exanic/fifo_tx.h>
#include <pcap.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

typedef struct exareplay_opt_s {
    /* input filename */
    char *input_name;

    char *device;
    u_int16_t port;

    /* replay two file at the same time */
    bool dualmode;

    /* skip large time interval in pcap */
    bool skip_interval;

    /* if 'skip_interval' is set TRUE, interval larger than it will be skipped */
    u_int32_t skip_interval_size;

} exareplay_opt_t;

typedef struct device_s {
    exanic_t *exanic;
    exanic_tx_t *tx;

} device_t;

typedef struct pcap_info_s {
    char **data;
    u_int32_t *len;
    u_int64_t *time_interval;
    uint32_t size;
} pcap_info_t;

typedef struct exareplay_s {
    exareplay_opt_t *opts;
    device_t *device;
    pcap_info_t *pcap_info;

} exareplay_t;

exareplay_t *exareplay_init();

void exareplay_free(exareplay_t *);

void device_init(exareplay_t *);

void device_close(exareplay_t *);

int exareplay_set_dualmode(exareplay_t *, bool);

int exareplay_parse_args(exareplay_t *, int argc, char *argv[]);

void exareplay_replay(exareplay_t *);

void pcap_info_init(exareplay_t *ctx);
void pcap_info_free(exareplay_t *ctx);
uint32_t get_pcap_size(char *filename);
