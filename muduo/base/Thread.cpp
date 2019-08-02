#include <muduo/base/Thread.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Exception.h>
#include <muduo/base/Logging.h>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo
{

namespace CurrentThread     // 每个线程私有的全局变量
{
    /* 线程局部全局变量, 若没有__thread修饰, 则是全局变量 */
    __thread int    t_cachedTid = 0;                            // 线程真实pid(tid)缓存
    __thread char   t_tidString[32];                            // tid字符串表现形式
    __thread const char *t_threadName = "unknown";              // 每个线程的名字
    const bool sameType = boost::is_same<int, pid_t>::value;    // 判断int与pid_t类型是否相等
    BOOST_STATIC_ASSERT(sameType);                              // 编译断言
}   // namespace CurrentThread

namespace detail
{

pid_t gettid()
{
    return static_cast<pid_t>(::syscall(SYS_gettid));    // 系统调用获得tid
}

void afterFork()
{
    //CurrentThread::t_cachedTid = 0;
    //CurrentThread::t_threadName = "main";
    /* 有些疑问, 为什么要在前面加muduo命名空间, 本身已经在muduo空间 */
    muduo::CurrentThread::t_cachedTid = 0;
    muduo::CurrentThread::t_threadName = "main";
    /* 这个没有加muduo命名空 */
    CurrentThread::tid();
    // no need to call pthread_atfork(NULL, NULL, &afterFork);
}

class ThreadNameInitializer
{
public:
    ThreadNameInitializer()
    {
        muduo::CurrentThread::t_threadName = "main";
        CurrentThread::tid();
        pthread_atfork(NULL, NULL, &afterFork);     // 创建线程后, 改变子线程的name, tid
    }
};

ThreadNameInitializer init;

}   // detail

using namespace muduo;      // 这行是否可以注释掉?

void
CurrentThread::cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = detail::gettid();
        int n = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
        assert(n == 6);
        (void)n;            // debug版本可以看到assert, release版本无assert, n警告未使用, 出错
    }
}

bool
CurrentThread::isMainThread()
{
    return tid() == ::getpid();
}

AtomicInt32 
Thread::numCreated_;    // 静态函数初始化

Thread::Thread(const ThreadFunc &func, const string &n)
    : started_(false),
    pthreadId_(0),
    tid_(0),
    func_(func),
    name_(n)
{
    numCreated_.increment();        // 线程数量自增1
}

Thread::~Thread()
{
    // no join
}

void 
Thread::start()
{
    assert(!started_);
    started_ = true;
    errno = pthread_create(&pthreadId_, NULL, &startThread, this);
    if (errno != 0)
    {
        LOG_SYSFATAL << "Failed in pthread_create";
    }
}

int
Thread::join()
{
    assert(started_);
    return pthread_join(pthreadId_, NULL);
}

void*
Thread::startThread(void *obj)
{
    Thread *thread = static_cast<Thread*>(obj);
    thread->runInThread();
    return NULL;
}

void
Thread::runInThread()
{
    tid_ = CurrentThread::tid();                        // 会缓存线程的私有属性, 即CurrentThread
    muduo::CurrentThread::t_threadName = name_.c_str(); // 会缓存线程的私有属性, 即CurrentThread
    try
    {
        func_();
        muduo::CurrentThread::t_threadName = "finished";
    }
    catch (const Exception& ex)
    {
        muduo::CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    }
    catch (const std::exception& ex)
    {
        muduo::CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...)
    {
        muduo::CurrentThread::t_threadName = "crashed";
        fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
        throw; // rethrow
    }
}

}   // namespace muduo