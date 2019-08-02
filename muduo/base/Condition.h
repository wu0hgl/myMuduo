#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
class Condition : boost::noncopyable
{
public:
    explicit Condition(MutexLock &mutex) : mutex_(mutex)    // 封装pthread_cond_inti
    {
        pthread_cond_init(&pcond_, NULL);
    }

    ~Condition()        // 封装pthread_cond_destroy
    {
        pthread_cond_destroy(&pcond_);
    }

    void wait()         // 等待满足条件, pthread_cond_wait封装
    {
        pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
    }

    // returns true if time out, false otherwise
    bool waitForSeconds(int seconds);

    void notify()       // pthread_cond_signal封装
    {
        pthread_cond_signal(&pcond_);
    }

    void notifyAll()    // pthread_cond_broadcast封装
    {
        pthread_cond_broadcast(&pcond_);
    }

private:
    MutexLock       &mutex_;    // 并不负责互斥锁的生命周期
    pthread_cond_t  pcond_;     // 条件变量， 条件变量加锁时会把互斥量解锁
};

}       // muduo

#endif  // MUDUO_BASE_CONDITION_H