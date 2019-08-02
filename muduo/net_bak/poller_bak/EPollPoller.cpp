#include <muduo/net/poller/EPollPoller.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>

#include <boost/static_assert.hpp>

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

using namespace muduo;
using namespace muduo::net;

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);

namespace
{
    const int kNew      = -1;
    const int kAdded    = 1;
    const int kDeleted  = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
    epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    int numEvents = ::epoll_wait(epollfd_,
        &*events_.begin(),
        static_cast<int>(events_.size()),
        timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);
        if (implicit_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size() * 2);             // 事件个数增长
        }
    }
    else if (numEvents == 0)
    {
        LOG_TRACE << " nothing happended";
    }
    else
    {
        LOG_SYSERR << "EPollPoller::poll()";
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents,
    ChannelList* activeChannels) const
{
    assert(implicit_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);  // update中event.data.ptr = channel; 所以可以返回通道
#ifndef NDEBUG
        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);                 // 返回到通道中
    }
}

void EPollPoller::updateChannel(Channel* channel)
{
    Poller::assertInLoopThread();                           // 断言处于IO线程中
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    const int index = channel->index();
    if (index == kNew || index == kDeleted)                 // Channel的index_初始化为-1
    {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        if (index == kNew)                                  // 未添加状态
        {
            assert(channels_.find(fd) == channels_.end());  // Channel中可以找到, 但channel_找不到
            channels_[fd] = channel;                        // 添加到channel_ map中
        }
        else // index == kDeleted                           // 已添加, 只是从epoll关注事件中删除的还保留在channel_ map中
        {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        (void)fd;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);                 // 仅仅表示从epoll关注中移除, 并没有从channel_ map中移除
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::removeChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);           // 断言等于kAdded或kDeleted, 而不能等于kNew(不在通道中)
    size_t n = channels_.erase(fd);                         // kAdded或kDeleted都要从channel_ map中移除
    (void)n;
    assert(n == 1);

    if (index == kAdded)                                    // kAdded表示在epoll关注中, kDeleted已经从epoll关注中移除了
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    bzero(&event, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_SYSERR << "epoll_ctl op=" << operation << " fd=" << fd;     // 并没有退出
        }
        else
        {
            LOG_SYSFATAL << "epoll_ctl op=" << operation << " fd=" << fd;
        }
    }
}
