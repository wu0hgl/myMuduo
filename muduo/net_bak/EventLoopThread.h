#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>

#include <boost/noncopyable.hpp>

namespace muduo
{

namespace net
{

class EventLoop;

class EventLoopThread : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback());
    ~EventLoopThread();
    EventLoop* startLoop(); // 通过调用Thread::start来启动线程, 该线程成为IO线程(会在这个线程中创建EventLoop对象)

private:
    void threadFunc();      //线程函数

    EventLoop   *loop_;     // loop_指向一个EventLoop对象, 一个IO线程有且只有一个EventLoop对象
    bool        exiting_;      //
    Thread      thread_;    // 使用基于对象的编程思想, 包含一个Thread对象. 若使用面向对象的编程思想应该继承Thread类
    MutexLock   mutex_;     // 与条件变量一起使用
    Condition   cond_;      // 用于等待线程函数threadFunc创建EventLoop对象
    ThreadInitCallback  callback_;  // 回调函数在EventLoop::loop循环之前被调用
};

}       // namespace net;

}       // namespace muduo

#endif  // MUDUO_NET_EVENTLOOPTHREAD_H