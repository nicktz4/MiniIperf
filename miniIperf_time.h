#ifndef __MINIIPERF_TIME_H
#define __MINIIPERF_TIME_H

#include <stdint.h>

/* usec -> μs*/
struct miniIperf_time {
    uint32_t sec;
    uint32_t usec;
};

int miniIperfTimeNow(struct miniIperf_time *);


uint64_t miniIperfTimeInUsec(struct miniIperf_time *);

double miniIperfTimeInSec(struct miniIperf_time *);

void miniIperfTimePrint(struct miniIperf_time *);

void miniIperfTimeDiff(struct miniIperf_time *, struct miniIperf_time *,struct miniIperf_time *);

#endif