#pragma once

#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#define PCAP_MAGIC_NANO (0xa1b23c4d) /* nanosecond format */
#define PCAP_MAGIC_USEC (0xa1b2c3d4) /* microsecond format */

#define MTU 1500

typedef struct pcap_hdr_s pcap_hdr_t;
typedef struct pcaprec_hdr_s pcaprec_hdr_t;
typedef struct pcap_file_s pcap_file_t;
typedef struct ts_s ts_t;

struct pcap_hdr_s {
    uint32_t magic_number;   /* magic number */
    uint16_t version_major;  /* major version number */
    uint16_t version_minor;  /* minor version number */
    int32_t thiszone;        /* GMT to local correction */
    uint32_t sigfigs;        /* accuracy of timestamps */
    uint32_t snaplen;        /* max length of captured packets, in octets */
    uint32_t network;        /* data link type */
};



struct ts_s{
        uint32_t tv_sec;         /* timestamp seconds */
        uint32_t tv_usec;        /* timestamp microseconds */
};

struct pcaprec_hdr_s {
   ts_t ts;                    /* timestamp */

    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t len;       /* actual length of packet */
};


struct pcap_file_s {

    pcap_hdr_t hdr;

    /* use in disk file */
    int fp;
    char buffer[MTU];

    /* use in memory */
    size_t data_len;
    char *data_ptr;
    
};

/**
 * @brief Opens a pcap file for reading
 * @param filename The name of the file to open
*/
pcap_file_t *mypcap_open(const char *filename);

/**
 * @brief Opens a pcap file from memory
 * @param data The data to read from
 * @param len The length of the data
*/
pcap_file_t *mypcap_open_memory(const char *data, size_t len);


/**
 * @brief Closes a pcap file
*/
void mypcap_close(pcap_file_t *pcap);


/**
 * @brief Reads the next packet from a pcap file
 * @param pcap The pcap file to read from
 * @param header The header of the packet
 * @return The packet data
*/
char* mypcap_next(pcap_file_t *pcap, pcaprec_hdr_t *header);

int mypcap_readnext(pcap_file_t *pcap, pcaprec_hdr_t *header, char* buffer);

char* mypcap_next_memory(pcap_file_t *pcap, pcaprec_hdr_t *header);
