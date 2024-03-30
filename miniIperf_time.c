
#include <stddef.h>
#include <stdio.h>

#include "miniIperf_time.h"

#include "time.h"

struct miniIperf_time miniIperfTimeNow() {

    struct timespec timesp;
    struct miniIperf_time now;

    int result;

    result = clock_gettime(CLOCK_MONOTONIC_RAW, &timesp);

    if(result==0)
    {
        now.sec = timesp.tv_sec;
        now.usec = timesp.tv_nsec / 1000;
    }
    return now;

}

uint64_t miniIperfTimeInUsec(struct miniIperf_time time) {

    return time.sec * 1000000LL + time.usec;

}

double miniIperfTimeInSec(struct miniIperf_time time) {

    return time.sec + time.usec/1000000.0;

}


void miniIperfTimePrint(struct miniIperf_time t)
{

    printf("%d.%06d\n", t.sec, t.usec);
}

struct miniIperf_time  miniIperfTimeDiff(struct miniIperf_time start, struct miniIperf_time end)
{
struct miniIperf_time diff;
    diff.sec = end.sec - start.sec;
    diff.usec = end.usec - start.usec;

    if( (int) diff.usec < 0)
    {
        diff.sec--;
        diff.usec += 1000000;
    }
    return diff;
}

