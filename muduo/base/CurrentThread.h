#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

namespace muduo
{

namespace CurrentThread
{
    extern __thread int    t_cachedTid;
    extern __thread char   t_tidString[32];     // Thread::runInThread                     
    extern __thread const char *t_threadName;   // Thread::runInThread

    void cacheTid();        // 设置t_tidString
    bool isMainThread();

    inline int tid()        // Thread::runInThread会调用此函数, 此时会设置t_cachTid与t_tidString
    {
        if (t_cachedTid == 0)
        {
            cacheTid();
        }
        return t_cachedTid;
    }

    inline const char* tidString()  // for logging
    {
        return t_tidString;
    }

    inline const char* name()
    {
        return t_threadName;
    }

}       // CurrentThread

}       // muduo

#endif  // MUDUO_BASE_CURRENTTHREAD_H