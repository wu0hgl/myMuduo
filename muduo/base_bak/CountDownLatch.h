#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>

namespace muduo
{

class CountDownLatch : boost::noncopyable
{
public:
    explicit CountDownLatch(int count);
    
    void wait();                // 互斥量加锁, 条件变量加锁(互斥量这时解锁)
    void countDown();           // 互斥量加锁计数-1
    int getCount() const;       // 互斥量加锁获得计数值

private:
    /* MutexLock与Condition声明顺序不可以改变 */
    mutable MutexLock   mutex_;     // mutable表示在const修饰的成员函数里可变
    Condition   condition_;         // 条件变量与互斥量配合使用, 初始化condition_时, mutex_必须先构造好
    int         count_;
};

}       // muduo

#endif  // MUDUO_BASE_COUNTDOWNLATCH_H