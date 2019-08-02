#ifndef MUDUO_NET_POLLER_H
#define MUDUO_NET_POLLER_H

#include <vector>
#include <boost/noncopyable.hpp>

#include <muduo/base/Timestamp.h>
#include <muduo/net/EventLoop.h>

namespace muduo
{

namespace net
{

class Channel;

///
/// Base class for IO Multiplexing
///
/// This class doesn't own the Channel objects.
class Poller : boost::noncopyable       // 一个EventLoop包含一个Poller
{
public:
    typedef std::vector<Channel*> ChannelList;

    Poller(EventLoop* loop);
    virtual ~Poller();

    /// Polls the I/O events.
    /// Must be called in the loop thread.
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0; // activeChannels传出参数, 返回触发的Channel数组

    /// Changes the interested I/O events.
    /// Must be called in the loop thread.
    virtual void updateChannel(Channel* channel) = 0;                       // 更新对应Channel的事件, 包括注册Channel

    /// Remove the channel, when it destructs.
    /// Must be called in the loop thread.
    virtual void removeChannel(Channel* channel) = 0;                       // 从Poller中移除对应的Channel

    static Poller* newDefaultPoller(EventLoop* loop);                       // 静态成员函数, 通过环境变量来确定使用PollPoller还是EPollPoller

    void assertInLoopThread()                                               // 确保在当前线程中
    {
        ownerLoop_->assertInLoopThread();
    }

private:
    EventLoop *ownerLoop_;          // Poller所属EventLoop
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_POLLER_H