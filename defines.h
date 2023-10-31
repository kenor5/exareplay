#pragma once

#define QWORD_ALIGN(x) (((x) + 7) & ~7)

/* differs in different host, you should test with*/
#define ticks_per_nano 1.79576209

#define DEBUG
#ifdef DEBUG
    #define LOG(fmt, args...) \
    do{\
        printf("%s %s:%d: ",__FILE__, __FUNCTION__, __LINE__);\
        printf(fmt, ##args);\
        fflush(stdout);\
        }while(0)
#else
    #define LOG(fmt, args...)
#endif

#define TX_SLOT_SIZE 4096
#define TX_SLOT_NUM 31

#define BANDWIDTH 10
#define time_delta 40
#define time_delta_burst_start 1390
#define burst_interval_min 480000000
#define burst_interval_max 510000000

/* when NUM/2 is sent out, preload first one*/
#define update_slot_until (TX_SLOT_NUM/2)
#define cur_update (TX_SLOT_NUM-update_slot_until-1)

#ifndef MTU
#define MTU 1500
#endif


#ifndef max
#define max(a, b)                                                                                                      \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a > _b ? _a : _b;                                                                                             \
    })
#endif

#ifndef min
#define min(a, b)                                                                                                      \
    ({                                                                                                                 \
        __typeof__(a) _a = (a);                                                                                        \
        __typeof__(b) _b = (b);                                                                                        \
        _a > _b ? _b : _a;                                                                                             \
    })
#endif

/* Time converters */
#define SEC_TO_MILLISEC(x) (x * 1000)
#define SEC_TO_MICROSEC(x) (x * 1000000)
#define SEC_TO_NANOSEC(x) ((u_int64_t)x * 1000000000)

#define MILLISEC_TO_SEC(x) (x / 1000)
#define MICROSEC_TO_SEC(x) (x / 1000000)
#define NANOSEC_TO_SEC(x) ((u_int64_t)x / 1000000000)

#define TIMEVAL_TO_MILLISEC(x) (((x)->tv_sec * 1000) + ((x)->tv_usec / 1000))
#define TIMEVAL_TO_MICROSEC(x) (((x)->tv_sec * 1000000) + (x)->tv_usec)
#define TIMEVAL_TO_NANOSEC(x) ((u_int64_t)((x)->tv_sec * 1000000000) + ((u_int64_t)(x)->tv_usec * 1000))
#define TIMSTAMP_TO_MICROSEC(x) (TIMEVAL_TO_MICROSEC(x))

#define MILLISEC_TO_TIMEVAL(x, tv)                                                                                     \
    do {                                                                                                               \
        (tv)->tv_sec = (x) / 1000;                                                                                     \
        (tv)->tv_usec = (x * 1000) - ((tv)->tv_sec * 1000000);                                                         \
    } while (0)

#define MICROSEC_TO_TIMEVAL(x, tv)                                                                                     \
    do {                                                                                                               \
        (tv)->tv_sec = (x) / 1000000;                                                                                  \
        (tv)->tv_usec = (x) - ((tv)->tv_sec * 1000000);                                                                \
    } while (0)

#define NANOSEC_TO_TIMEVAL(x, tv)                                                                                      \
    do {                                                                                                               \
        (tv)->tv_sec = (x) / 1000000000;                                                                               \
        (tv)->tv_usec = ((x) % 1000000000) / 1000;                                                                     \
    } while (0)

#define NANOSEC_TO_TIMESPEC(x, ts)                                                                                     \
    do {                                                                                                               \
        (ts)->tv_sec = (x) / 1000000000;                                                                               \
        (ts)->tv_nsec = (x) % 1000000000;                                                                              \
    } while (0)

#define TIMESPEC_TO_MILLISEC(x) (((x)->tv_sec * 1000) + ((x)->tv_nsec / 1000000))
#define TIMESPEC_TO_MICROSEC(x) (((x)->tv_sec * 1000000) + (x)->tv_nsec / 1000)
#define TIMESPEC_TO_NANOSEC(x) ((u_int64_t)((x)->tv_sec * 1000000000) + ((u_int64_t)(x)->tv_nsec))

#define TIMEVAL_SET(a, b)                                                                                              \
    do {                                                                                                               \
        (a)->tv_sec = (b)->tv_sec;                                                                                     \
        (a)->tv_usec = (b)->tv_usec;                                                                                   \
    } while (0)

#define TIMESPEC_SET(a, b)                                                                                             \
    do {                                                                                                               \
        (a)->tv_sec = (b)->tv_sec;                                                                                     \
        (a)->tv_nsec = (b)->tv_nsec;                                                                                   \
    } while (0)
