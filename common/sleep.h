// #pragma once
// #include "rdtsc.h"

// /* pretty damn accurate. */
// static inline void
// rdtsc_sleep(unsigned long long ticks)
// {
//     unsigned long long start = rdtsc();
//     register unsigned long long end = start + ticks;
//     while (rdtsc() < end)
//         ;
// }