#pragma once

#include <sys/types.h>
#include <stdbool.h>
#include "defines.h"

typedef struct exareplay_opt_s{
    /* input filename */
    char *input_name;

    bool dualmode;
} exareplay_opt_t;

typedef struct exareplay_s
{
    exareplay_opt_t *opts;
} exareplay_t;


exareplay_t* exareplay_init();

int exareplay_set_dualmode(exareplay_t*, bool);

int exareplay_parse_args(exareplay_t*, int argc);
