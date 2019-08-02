#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include <muduo/base/CurrentThread.h>
#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{

class MutexLock : boost::noncopyable
{

public:
    MutexLock() : holder_(0)
    {
        int ret = pthread_mutex_init(&mutex_, NULL);
        assert(ret == 0);   (void) ret;
    }

    ~MutexLock()                    // 析构函数销毁锁时会先确保hold_=0, 即已经解锁
    {
        assert(holder_ == 0);
        int ret = pthread_mutex_destroy(&mutex_);
        assert(ret == 0);   (void) ret;
    }

    bool isLockByThisThread()       // 互斥锁是否被当前线程持有
    {
        return holder_ == CurrentThread::tid();
    }

    void assertLocked()
    {
        assert(isLockByThisThread());
    }

    // internal usage

    void lock()                     // 加锁时会获得加锁线程的tid存放在成员变量holder_中
    {
        pthread_mutex_lock(&mutex_);
        holder_ = CurrentThread::tid();
    }

    void unlock()                   // 解锁时会令hold_=0
    {
        holder_ = 0;
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t* getPthreadMutex()  // non-const
    {
        return &mutex_;
    }

private:
    pthread_mutex_t mutex_;
    pid_t           holder_;        // 互斥锁持有的线程
};

class MutexLockGuard : boost::noncopyable
{
public:
    explicit MutexLockGuard(MutexLock &mutex)   // 构造函数加锁
        : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~MutexLockGuard()                           // 析构函数解锁
    {
        mutex_.unlock();
    }

private:
    MutexLock &mutex_;
};

}   // namespace muduo

// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name" // 定义一个宏放置错误使用MutexGuard类

#endif  // MUDUO_BASE_MUTEX_H