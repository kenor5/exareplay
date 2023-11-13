#pragma once

#include "exareplay_api.h"
#include "slot.h"
#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef DEBUG
#define DEBUG
#endif

void *thread_disk2memory(void *args);

void *thread_memory2NIC(void *);

void *thread_NICsend(void *);

// void start_threads(long, void *(*start_routine)(void *));

// void join_threads();