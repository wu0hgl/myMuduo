#include <muduo/base/Timestamp.h>

#include <sys/time.h>
#include <stdio.h>
#define __STD_FORMAT_MACROS
#include <inttypes.h>
#undef __STD_FORMAT_MACROS

#include <boost/static_assert.hpp>

using namespace muduo;

BOOST_STATIC_ASSERT(sizeof(Timestamp) == sizeof(int64_t));      // 利用boost库编译时断言

Timestamp::Timestamp(int64_t microSecondsSinceEpoch) 
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
{

}

string 
Timestamp::toString() const
{
    char    buf[32] = { 0 };
    int64_t second = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", second, microseconds);
    return buf;
}

string 
Timestamp::toFormattedString() const
{
    char    buf[32] = { 0 };
    time_t  seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    int     microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
    struct tm tm_time;
    /* 通过秒转换成时间结果体 */
    gmtime_r(&seconds, &tm_time);   // gmtime_r表示是一个线程安全的函数

    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
        microseconds);
    return buf;
}

Timestamp 
Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);    // 获取当前时间, 第二个参数为时区一般为空
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

Timestamp 
Timestamp::invalid()
{
    return Timestamp();
}