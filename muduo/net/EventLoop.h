﻿#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/TimerId.h>

namespace muduo
{

namespace net
{

class Channel;
class Poller;
class TimerQueue;
///
/// Reactor, at most one per thread.
///
/// This is an interface class, so don't expose too much details.
class EventLoop : boost::noncopyable
{
public:
    typedef boost::function<void()> Functor;    // 定义绑定函数类型

    EventLoop();
    ~EventLoop();           // force out-line dtor, for scoped_ptr members.

    ///
    /// Loops forever.
    ///
    /// Must be called in the same thread as creation of the object.
    ///
    void loop();            // 事件循环

    void quit();            // 可以跨线程调用

    ///
    /// Time when poll returns, usually means data arrivial.
    ///
    Timestamp pollReturnTime() const { return pollReturnTime_; }    // 返回Poller触发时间

    /// Runs callback immediately in the loop thread.
    /// It wakes up the loop, and run the cb.
    /// If in the same loop thread, cb is run within the function.
    /// Safe to call from other threads.
    void runInLoop(const Functor& cb);
    /// Queues callback in the loop thread.
    /// Runs after finish pooling.
    /// Safe to call from other threads.
    void queueInLoop(const Functor& cb);

    /* timers */
    ///
    /// Runs callback at 'time'.
    /// Safe to call from other threads. 跨线程调用
    ///
    TimerId runAt(const Timestamp& time, const TimerCallback& cb);
    ///
    /// Runs callback after @c delay seconds.
    /// Safe to call from other threads.
    ///
    TimerId runAfter(double delay, const TimerCallback& cb);
    ///
    /// Runs callback every @c interval seconds.
    /// Safe to call from other threads.
    ///
    TimerId runEvery(double interval, const TimerCallback& cb);
    ///
    /// Cancels the timer.
    /// Safe to call from other threads.
    ///
    void cancel(TimerId timerId);

    // internal usage
    void wakeup();      // 唤醒当前线程, 因为跨线程调用quit时, 当前线程可能阻塞在poller_或handleEvent位置, 此时需唤醒操作
    void updateChannel(Channel* channel);		// 在Poller中添加或者更新通道
    void removeChannel(Channel* channel);		// 从Poller中移除通道

    void assertInLoopThread()
    {
        if (!isInLoopThread())
        {   
            abortNotInLoopThread();
        }
    }
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    static EventLoop* getEventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();
    void handleRead();  // waked up
    void doPendingFunctors();

    void printActiveChannels() const; // DEBUG

    typedef std::vector<Channel*> ChannelList;

    bool looping_;                  /* atomic */
    bool quit_;                     /* atomic */
    bool eventHandling_;            /* atomic */
    bool callingPendingFunctors_;   /* atomic */
    const pid_t     threadId_;		        // 当前对象所属线程ID
    Timestamp       pollReturnTime_;
    boost::scoped_ptr<Poller> poller_;      // 智能指针, 负责Poller的生命周期
    boost::scoped_ptr<TimerQueue> timerQueue_;
    int wakeupFd_;				            // 用于eventfd所创建的文件描述符
    // unlike in TimerQueue, which is an internal class,
    // we don't expose Channel to client.
    boost::scoped_ptr<Channel> wakeupChannel_;  // 该通道将会纳入poller_来管理, 与EventLoop构成组合的关系, EventLoop只负责wakeupChannel_生存期
    ChannelList     activeChannels_;            // Poller返回的活动通道
    Channel         *currentActiveChannel_;     // 当前正在处理的活动通道, 用于从activeChannels_取通道
    MutexLock mutex_;
    std::vector<Functor> pendingFunctors_;      // @BuardedBy mutex_
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_EVENTLOOP_H