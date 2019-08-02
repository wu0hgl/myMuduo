#include <muduo/base/Logging.h>

#include <muduo/base/CurrentThread.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Timestamp.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

namespace muduo
{

/*
class LoggerImpl
{
public:
typedef Logger::LogLevel LogLevel;
LoggerImpl(LogLevel level, int old_errno, const char* file, int line);
void finish();

Timestamp time_;
LogStream stream_;
LogLevel level_;
int line_;
const char* fullname_;
const char* basename_;
};
*/

__thread char t_errnobuf[512];  // 错误字符串缓存
__thread char t_time[32];       // 时间缓存
__thread time_t t_lastSecond;   // 存储从1970年到现在经过了多少秒, 应该是某个整型变量

const char* strerror_tl(int savedErrno)
{
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

Logger::LogLevel initLogLevel()
{
    return Logger::TRACE;
    /*
    if (::getenv("MUDUO_LOG_TRACE"))        // 获取环境变量
    return Logger::TRACE;
    else if (::getenv("MUDUO_LOG_DEBUG"))
    return Logger::DEBUG;
    else
    return Logger::INFO;
    */
}

Logger::LogLevel g_logLevel = initLogLevel();   // 全局变量, 在main函数之前执行initLogLevel获取日志级别

const char* LogLevelName[Logger::NUM_LOG_LEVELS] =  // Logger::NUM_LOG_LEVELS=6
{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};

// helper class for known string length at compile time
class T     // 帮助类, 用于确保字符串长度与移植字符串长度相等
{
public:
    T(const char* str, unsigned len)
        :str_(str),
        len_(len)
    {
        assert(strlen(str) == len_);
    }

    const char* str_;
    const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v) // 用于如: stream_ << T(CurrentThread::tidString(), 6);
{
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) // 用于如: stream_ << " - " << basename_ 
{
    s.append(v.data_, v.size_);
    return s;
}

void defaultOutput(const char* msg, int len)    // 默认输出到标准输出
{
    size_t n = fwrite(msg, 1, len, stdout); 
    //FIXME check n
    (void)n;
}

void defaultFlush()                             // 默认刷新标准输出
{
    fflush(stdout);
}

// 下面两个都是函数指针类型 全局 变量
Logger::OutputFunc g_output = defaultOutput;    // 定义全局标准输出函数
Logger::FlushFunc  g_flush  = defaultFlush;     // 定义全局刷新函数

}   // namespace muduo

using namespace muduo;

Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
    : time_(Timestamp::now()),      // 登记当前时间
    stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
    formatTime();                   // 格式化时间输出到stream_中
    CurrentThread::tid();           // 缓存当前线程ID, 要先调用
    stream_ << T(CurrentThread::tidString(), 6);    // 线程id输出到stream_中
    stream_ << T(LogLevelName[level], 6);           // 日志级别输出到stream_中
    if (savedErrno != 0)                            // 错误信息到stream_
    {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

void Logger::Impl::formatTime()
{
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t  seconds = static_cast<time_t>(microSecondsSinceEpoch / 1000000);
    int     microseconds = static_cast<int>(microSecondsSinceEpoch % 1000000);
    if (seconds != t_lastSecond)
    {
        t_lastSecond = seconds;
        struct tm tm_time;
        ::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                           tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                           tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17); (void)len;
    }
    Fmt us(".%06dZ ", microseconds);    // 微秒格式化类
    assert(us.length() == 9);
    stream_ << T(t_time, 17) << T(us.data(), 9);
}

void Logger::Impl::finish()
{
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << ' ';   // 格式化函数名称
}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line)     // toAbort为true, 级别为FATAL, 否则基本为ERROR
{
}

Logger::~Logger()
{
    impl_.finish();
    const LogStream::Buffer& buf(stream().buffer());        // stream().buffer()返回引用, 并没有拷贝缓存区
    g_output(buf.data(), buf.length());
    if (impl_.level_ == FATAL)
    {
        g_flush();          // 终止程序之前刷新一下
        abort();            // Level为FATAL要退出程序
    }
}

void Logger::setLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}