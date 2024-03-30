#ifndef __MINIIPERF_TIME_H
#define __MINIIPERF_TIME_H

#include <stdint.h>

/* usec -> μs*/
struct miniIperf_time {
    uint32_t sec;
    uint32_t usec;
};

struct miniIperf_time miniIperfTimeNow();


uint64_t miniIperfTimeInUsec(struct miniIperf_time );

double miniIperfTimeInSec(struct miniIperf_time );

void miniIperfTimePrint(struct miniIperf_time);

struct miniIperf_time  miniIperfTimeDiff(struct miniIperf_time start, struct miniIperf_time end);

#endif