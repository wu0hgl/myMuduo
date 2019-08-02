#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include <muduo/base/LogStream.h>
#include <muduo/base/Timestamp.h>

namespace muduo
{

class Logger
{
public:
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    // compile time calculation of basename of source file
    class SourceFile    // Logger类内部嵌套类, 用于保存文件名
    {
    public:
        template<int N>
        inline SourceFile(const char(&arr)[N])
            : data_(arr),
            size_(N - 1)
        {
            const char* slash = strrchr(data_, '/'); // builtin function
            if (slash)
            {
                data_ = slash + 1;                  // 指向第一个字符
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        explicit SourceFile(const char* filename)
            : data_(filename)
        {
            const char* slash = strrchr(filename, '/');  // 从后向前找第一个 / 字符位置
            if (slash)
            {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));    // 从data_开始计算文件名长度
        }

        const char* data_;
        int         size_;
    };

    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char* func);
    Logger(SourceFile file, int line, bool toAbort);
    ~Logger();          // 格式化信息都在impl_.stream_, Logger析构函数用于输出字符串

    LogStream& stream() { return impl_.stream_; }

    static LogLevel logLevel();                 // 静态成员函数, 获取日志级别
    static void setLogLevel(LogLevel level);    // 静态成员函数, 设置日志级别

    typedef void(*OutputFunc)(const char* msg, int len);    // 定义更改输出函数的函数类型
    typedef void(*FlushFunc)();                             // 定义刷新缓存函数类型

    static void setOutput(OutputFunc);  // 更改输出函数
    static void setFlush(FlushFunc);    // 更改刷新函数

private:
    class Impl      // Logger类内部嵌套类, 负责具体字符串格式化
    {
    public:
        typedef Logger::LogLevel LogLevel;
        Impl(LogLevel level, int old_errno, const SourceFile& file, int line);  // 引用类型, 并没有构造一个新的SourceFile对象
        void formatTime();      // 格式化时间
        void finish();          // 连接文件名和行号

        Timestamp   time_;        // 登记时间
        LogStream   stream_;      // 内部组合LogStream, 用于缓存格式化之后的字符串
        LogLevel    level_;
        int         line_;
        SourceFile  basename_;
    };

    Impl impl_;     // Logger内只有这一个属性
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
    return g_logLevel;
}

// 木铎日志库的三个级别
// Logger函数构造完成之后返回impl_.stream_引用, 把字符串格式化进去
#define LOG_TRACE if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()    // 构造匿名Logger对象, 调用后析构掉

#define LOG_DEBUG if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()

#define LOG_INFO if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()

#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()        // false, 不会退出程序
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()       // true, 会退出程序

const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.
// 宏定义反斜号后面不能有注释, 空格也不可以
#define CHECK_NOTNULL(val) \
::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char *names, T* ptr) {
    if (ptr == NULL) {
        Logger(file, line, Logger::FATAL).stream() << names;
    }
    return ptr;
}

}       // namespace muduo

#endif  // MUDUO_BASE_LOGGING_H