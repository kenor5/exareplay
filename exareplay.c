#include "exareplay.h"
#include "exareplay_api.h"
#include <stdlib.h>
#include <stdio.h>

#ifndef DEBUG
#define DEBUG
#endif

exareplay_t *ctx;

int
main(int argc, char* argv[]) {

    int c;

    fflush(NULL);

    ctx = exareplay_init();

    
}