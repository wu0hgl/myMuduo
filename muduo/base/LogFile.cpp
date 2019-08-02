#include <muduo/base/LogFile.h>
#include <muduo/base/Logging.h> // strerror_tl
#include <muduo/base/ProcessInfo.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace muduo;

// not thread safe
class LogFile::File : boost::noncopyable
{
public:
    explicit File(const string& filename)
        : fp_(::fopen(filename.data(), "ae")),
        writtenBytes_(0)
    {
        assert(fp_);
        ::setbuffer(fp_, buffer_, sizeof buffer_);  // 设置文件指针缓冲区, 超过这个大小会自动flush到文件中
        // posix_fadvise POSIX_FADV_DONTNEED ?
    }

    ~File()
    {
        ::fclose(fp_);
    }

    void append(const char* logline, const size_t len)
    {
        size_t n = write(logline, len);     // 调用内部的write成员函数
        size_t remain = len - n;
        // remain>0表示没写完，需要继续写直到写完
        while (remain > 0)
        {
            size_t x = write(logline + n, remain);
            if (x == 0)
            {
                int err = ferror(fp_);
                if (err)
                {
                    fprintf(stderr, "LogFile::File::append() failed %s\n", strerror_tl(err));
                }
                break;
            }
            n += x;
            remain = len - n; // remain -= x
        }

        writtenBytes_ += len;
    }

    void flush()
    {
        ::fflush(fp_);
    }

    size_t writtenBytes() const { return writtenBytes_; }

private:

    size_t write(const char* logline, size_t len)
    {
#undef fwrite_unlocked
        return ::fwrite_unlocked(logline, 1, len, fp_); // 不加锁的方式写入
    }

    FILE* fp_;                  // 文件指针
    char buffer_[64 * 1024];    // 文件指针缓冲区
    size_t writtenBytes_;       // 已经写入的字节数
};

LogFile::LogFile(const string& basename,
                 size_t rollSize,
                 bool threadSafe,
                 int flushInterval)
    : basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
    assert(basename.find('/') == string::npos);
    rollFile();         // 这里调用实际是为了产生文件
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, int len)
{
    if (mutex_)
    {
        MutexLockGuard lock(*mutex_);
        append_unlocked(logline, len);
    }
    else
    {
        append_unlocked(logline, len);
    }
}

void LogFile::flush()
{
    if (mutex_)
    {
        MutexLockGuard lock(*mutex_);
        file_->flush();
    }
    else
    {
        file_->flush();
    }
}

void LogFile::append_unlocked(const char* logline, int len)
{
    file_->append(logline, len);    // 写入到文件中

    if (file_->writtenBytes() > rollSize_)
    {
        rollFile();     // 根据文件大小来滚动日志
    }
    else
    {
        if (count_ > kCheckTimeRoll_)
        {
            count_ = 0;
            time_t now = ::time(NULL);
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_; // 当前时间取整
            if (thisPeriod_ != startOfPeriod_)                              // 到达第二天时间
            {
                rollFile();         // 第二天滚动日志
            }
            else if (now - lastFlush_ > flushInterval_) // 到达间隔时间进行flush
            {
                lastFlush_ = now;
                file_->flush();
            }
        }
        else
        {
            ++count_;
        }
    }
}

void LogFile::rollFile()
{
    time_t now = 0;
    string filename = getLogFileName(basename_, &now);
    // 注意，这里先除kRollPerSeconds_ 后乘kRollPerSeconds_表示
    // 对齐至kRollPerSeconds_整数倍，也就是时间调整到当天零点。
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;   // 第一个除法有取整操作

    if (now > lastRoll_)    // now > lastRoll_ 需要滚动日志
    {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new File(filename));    // 创建新的文件
    }
}

string LogFile::getLogFileName(const string& basename, time_t* now)
{
    string filename;
    filename.reserve(basename.size() + 64); // 保留文件名+64个字符的空间
    filename = basename;

    char timebuf[32];
    char pidbuf[32];
    struct tm tm;
    *now = time(NULL);  // 距离1970年...的时间
    gmtime_r(now, &tm); // FIXME: localtime_r ?
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);  // 时间格式化
    filename += timebuf;
    filename += ProcessInfo::hostname();
    snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
    filename += pidbuf;
    filename += ".log";

    return filename;
}