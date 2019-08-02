#include <muduo/net/EventLoop.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/Poller.h>
#include <muduo/net/TimerQueue.h>

//#include <poll.h>
#include <boost/bind.hpp>

#include <signal.h>
#include <sys/eventfd.h>

using namespace muduo;
using namespace muduo::net;

namespace
{

// 当前线程EventLoop对象指针
// 线程局部存储, 每个线程都有一个这个指针对象, 否则这个对象就是共享的
__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_SYSERR << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
        LOG_TRACE << "Ignore SIGPIPE";
    }
};
#pragma GCC diagnostic error "-Wold-style-cast"

/* 匿名命名空间全局函数, 在main函数执行之前调用, 即在程序开始执行时, 就已经忽略SIGPIPE信号 */
IgnoreSigPipe initObj;  

}   // namespace

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop()
    : looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(NULL)
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    // 如果当前线程已经创建了EventLoop对象，终止(LOG_FATAL)
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
            << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    wakeupChannel_->setReadCallback(
        boost::bind(&EventLoop::handleRead, this));     // wakeupChannel_回调函数
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}

/* 事件循环, 该函数不能跨线程调用, 只能在创建该对象的线程中调用 */
///
/// Loops forever.
///
/// Must be called in the same thread as creation of the object.
///
void EventLoop::loop()
{
    assert(!looping_);
    // 断言当前处于创建该对象的线程中
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";

    //::poll(NULL, 0, 5 * 1000);      // 等待5s没有做任何事
    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);     // 超时时间10s, 调用poller_返回活动的通道
        //++iteration_;
        if (Logger::logLevel() <= Logger::TRACE)
        {
            printActiveChannels();
        }
        // TODO sort channel by priority
        eventHandling_ = true;
        for (ChannelList::iterator it = activeChannels_.begin();
            it != activeChannels_.end(); ++it)
        {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent(pollReturnTime_);            // 调用回调函数处理通道, 如果不是在IO线程中调用quit,
        }                                                                   // 而是在其他线程中调用quit, 这时IO线程不能马上处理完毕
        currentActiveChannel_ = NULL;
        eventHandling_ = false;
        doPendingFunctors();        // 其他线程或当前线程添加的一些回调任务, IO线程也可以执行一些计算任务, 否则当IO不频繁时一直处于阻塞状态
    }

    LOG_TRACE << "EventLoop " << this << "stop looping";
    looping_ = false;
}

// 该函数可以跨线程调用
void EventLoop::quit()
{
    quit_ = true;       // quit_是bool类型, 原子性操作, 跨线程调用时, 该线程可能阻塞在poll或handleEvent位置, 此时需唤醒操作
    if (!isInLoopThread())
    {
        wakeup();     // 如果不是在当前线程调用quit, 先唤醒该线程
    }
}

// 在I/O线程中执行某个回调函数，该函数可以跨线程调用
void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
        // 如果是当前IO线程调用runInLoop，则同步调用cb
        cb();
    }
    else
    {
        // 如果是其它线程调用runInLoop，则异步地将cb添加到队列
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }

    // 调用queueInLoop的线程不是当前IO线程需要唤醒, 以便IO线程及时执行任务
    // 或者调用queueInLoop的线程是当前IO线程，并且此时正在调用pending functor，需要唤醒(当前线程调用doPendingFunctors内部又调用queueInLoop需唤醒, 若不唤醒无法处理后添加的queueInLoop)
    // 只有IO线程的事件回调中调用queueInLoop才不需要唤醒, 因为handleEvent执行后接下来会执行doPendingFunctors
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);           // 断言channel属于本EventLoop负责
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||
            std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
        << " was created in threadId_ = " << threadId_
        << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    //ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);               // 向wakeupFd_中写入8个字节就可唤醒另一个等待的线程
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    //ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

/*
不是简单地在临界区内依次调用Functor，而是把回调列表swap到functors中，这样一方面减小了临界区的长度（意味着不会阻塞其它线程的queueInLoop()），另一方面，也避免了死锁（因为Functor可能再次调用queueInLoop()）

由于doPendingFunctors()调用的Functor可能再次调用queueInLoop(cb)，这时，queueInLoop()就必须wakeup()，否则新增的cb可能就不能及时调用了

muduo没有反复执行doPendingFunctors()直到pendingFunctors为空，这是有意的，否则IO线程可能陷入死循环，无法处理IO事件。 交换的任务执行完毕就可以

*/
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
    for (ChannelList::const_iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
        const Channel* ch = *it;
        LOG_TRACE << "{" << ch->reventsToString() << "} ";
    }
}
