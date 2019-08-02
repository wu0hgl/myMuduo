#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <deque>
#include <assert.h>

namespace muduo
{

template<typename T>
class BlockingQueue : boost::noncopyable
{
public:
    BlockingQueue()
        : mutex_(),
        notEmpty_(mutex_),
        queue_()
    {
    }

    void put(const T &x)        // 加入队列中
    {
        MutexLockGuard lock(mutex_);
        queue_.push_back(x);
        notEmpty_.notify();     // 可以移动到外面, 如下
        //{
        //    MutexLockGuard lock(mutex_);
        //    queue_.push_back(x);
        //}
        //notEmpty_.notify();
    }

    T take()                    // 从队列中取出元素
    {
        MutexLockGuard lock(mutex_);
        // always use a while-loop, due to spurious wakeup, 防止虚假唤醒?
        while (queue_.empty())
        {
            notEmpty_.wait();
        }
        assert(!queue_.empty());
        T front(queue_.front());
        queue_.pop_front();
        return front;
    }

    size_t size() const
    {
        MutexLockGuard lock(mutex_);
        return queue_.size();
    }

private:
    mutable MutexLock   mutex_;     // 互斥锁, 与MuteLockGuard配合使用
    Condition           notEmpty_;  // 由于是无界缓冲队列, 只需队列不为空时条件变量
    std::deque<T>       queue_;
};

}       // muduo

#endif  // MUDUO_BASE_BLOCKINGQUEUE_H