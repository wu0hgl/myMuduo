#include <muduo/net/EventLoopThreadPool.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
    : baseLoop_(baseLoop),
    started_(false),
    numThreads_(0),
    next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // Don't delete loop, it's stack variable
}

/* 启动线程池 */
void EventLoopThreadPool::start(const ThreadInitCallback& cb)   // ThreadInitCallback, 当启动EventLoopThread即IO线程进入循环之前会调用cb
{
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        EventLoopThread* t = new EventLoopThread(cb);
        threads_.push_back(t);
        loops_.push_back(t->startLoop());   // 启动EventLoopThread线程，在进入事件循环之前，会调用cb
    }
    if (numThreads_ == 0 && cb)             // 没有创建这些线程并且cb不为空
    {
        // 只有一个EventLoop，在这个EventLoop进入事件循环之前，调用cb
        cb(baseLoop_);
    }
}

/* 新连接到来时, 要选择一个EventLoop对象进行处理 */
EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    EventLoop* loop = baseLoop_;        // acceptor所属loop, 即mainReactor

    // 如果loops_为空，则loop指向baseLoop_
    // 如果不为空，按照round-robin（RR，轮叫）的调度方式选择一个EventLoop
    if (!loops_.empty())                // loops_为空则没有创建出新的线程, 所选择的EventLoop对象还是mainReactor的EventLoop, 就是单线程模式
    {                                   // 这时不仅处理监听套接字还处理已连接套接字
        // round-robin
        loop = loops_[next_];
        ++next_;                        // 下一个EventLoop下标
        if (implicit_cast<size_t>(next_) >= loops_.size())
        {
            next_ = 0;                  // 如果大于loops_.size()则从0重新算起
        }
    }
    return loop;
}
