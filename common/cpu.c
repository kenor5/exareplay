#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

// 打印指定线程的绑核id
void print_cpu_affinity_id(int core_nums, pthread_t tid, cpu_set_t* cpu_set)
{
    int i = 0;
    for (i = 0; i < core_nums; i++)
    {
        if (CPU_ISSET(i, cpu_set))
        {
            printf("thread[%x] is running in processor %d\n", (unsigned int)tid, i);
        }
    }
}

// 获取指定线程的绑核id
int frank_phtread_cpu_affinity_get(pthread_t tid, cpu_set_t* cpu_set)
{
    CPU_ZERO(cpu_set);
    if (pthread_getaffinity_np(tid, sizeof(cpu_set_t), cpu_set) < 0)
    {
        fprintf(stderr, "get thread[%x] affinity failed\n", (unsigned int)tid);
        return 1;
    }

    return 0;
}

// 指定线程绑核
int frank_pthread_single_cpu_affinity_set(int core_id, pthread_t tid)
{
    cpu_set_t mask;

    CPU_ZERO(&mask);
    CPU_SET(core_id, &mask);
    if (pthread_setaffinity_np(tid, sizeof(cpu_set_t), &mask) < 0)
    {
        fprintf(stderr, "set thread[%x] affinity failed\n", (unsigned int)tid);
        return 1;
    }

    return 0;
}
