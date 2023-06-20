#include "Timestamp.h"

#include <sys/time.h>

Timestamp::Timestamp() : microSecondsSinceEpoch_(0)
{

}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch) 
                    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{

}

Timestamp Timestamp::now()
{
    // return Timestamp(time(NULL));
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

std::string Timestamp::toString() const
{
    char buf[128] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_/kMicroSecondsPerSecond);
    struct tm tm_time;
    gmtime_r(&seconds, &tm_time);
    snprintf(buf, 128, "%04d/%02d/%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday,
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec);
    return buf;
}

