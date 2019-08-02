#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
// 可以实现线程安全的添加日志, 多个线程使用同一个对象进行添加
class LogFile : boost::noncopyable
{
public:
    LogFile(const string& basename,
            size_t rollSize,
            bool threadSafe = true,     // 默认线程安全
            int flushInterval = 3);
    ~LogFile();

    void append(const char* logline, int len);  // 将len长的logline字符添加到日志中
    void flush();                               // 刷新到文件中

private:
    void append_unlocked(const char* logline, int len); // 不加锁的方式添加

    static string getLogFileName(const string& basename, time_t* now);  // 获取日志文件的名称
    void rollFile();            // 滚动日志

    const string basename_;     // 日志文件basename
    const size_t rollSize_;     // 日志文件达到rolSize_换一个新文件
    const int flushInterval_;   // 日志写入间隔时间

    int count_;                 // 计数器, 初始值为0, 达到kCheckTimeRoll_是否换一个新的日志, 或者达到roolSize_

    boost::scoped_ptr<MutexLock> mutex_;
    time_t startOfPeriod_;      // 开始记录日志时间（调整至零点的时间, 方便下一次日志滚动）
    time_t lastRoll_;           // 上一次滚动日志文件时间
    time_t lastFlush_;          // 上一次日志写入文件时间
    class File;                 // 前向声明
    boost::scoped_ptr<File> file_;  // 智能指针

    const static int kCheckTimeRoll_ = 1024;
    const static int kRollPerSeconds_ = 60 * 60 * 24;
};

}

#endif  // MUDUO_BASE_LOGFILE_H