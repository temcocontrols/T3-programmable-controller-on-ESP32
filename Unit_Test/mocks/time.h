#ifndef MOCK_TIME_H
#define MOCK_TIME_H

#define timezone ucrt_timezone
#include_next <time.h>
#undef timezone

#include <sys/time.h>

static inline struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
    struct tm *p = gmtime(timep);
    if (p && result) {
        *result = *p;
        return result;
    }
    return NULL;
}

static inline int settimeofday(const struct timeval *tv, const void *tz)
{
    (void)tv; (void)tz;
    return 0;
}

#endif // MOCK_TIME_H
