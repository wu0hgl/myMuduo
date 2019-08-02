#include <muduo/net/poller/PollPoller.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Types.h>
#include <muduo/net/Channel.h>

#include <assert.h>
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

PollPoller::PollPoller(EventLoop* loop)
    : Poller(loop)
{
}

PollPoller::~PollPoller()
{
}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)  // activeChannels传出参数
{
    // XXX pollfds_ shouldn't change
    int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);  // 返回通道里面去
    }
    else if (numEvents == 0)
    {
        LOG_TRACE << " nothing happended";
    }
    else
    {
        LOG_SYSERR << "PollPoller::poll()";
    }
    return now;
}

void PollPoller::fillActiveChannels(int numEvents,
    ChannelList* activeChannels) const
{
    for (PollFdList::const_iterator pfd = pollfds_.begin();
        pfd != pollfds_.end() && numEvents > 0; ++pfd)
    {
        if (pfd->revents > 0)
        {
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());          // 通道必须存在, 否则无法查找通道里的文件描述符
            Channel* channel = ch->second;
            assert(channel->fd() == pfd->fd);       // 断言channel里的fd和pfd里的fd相等
            channel->set_revents(pfd->revents);
            // pfd->revents = 0;
            activeChannels->push_back(channel);     // 为了EventLoop中的activeChannels_进行相应的处理
        }
    }
}

void PollPoller::updateChannel(Channel* channel)    // 用于注册或者更新通道
{
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    if (channel->index() < 0)
    {
        // index < 0说明是一个新的通道, Channel的index_初始化为-1
        // a new one, add to pollfds_
        assert(channels_.find(channel->fd()) == channels_.end());   // 断言新通道找不到
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size()) - 1;            // 添加新通道的索引为当前通道数量-1
        channel->set_index(idx);                                    // 更新chennel的索引
        channels_[pfd.fd] = channel;
    }
    else
    {
        // update existing one
        assert(channels_.find(channel->fd()) != channels_.end());   // 断言更新的通道已存在
        assert(channels_[channel->fd()] == channel);                // 以fd为索引的channel为更新的channel
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));// 索引大于0, 小于pollfds_的数量
        struct pollfd& pfd = pollfds_[idx];                         // 使用引用不需要拷贝
        assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd() - 1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        // 将一个通道暂时更改为不关注事件，但不从Poller中移除该通道
        if (channel->isNoneEvent())
        {
            // ignore this pollfd
            // 暂时忽略该文件描述符的事件, 只是把文件描述符变为负数-1就表示不关注, 并没有从poll所关注的数组中移除
            // 这里pfd.fd 可以直接设置为-1, 负数不是一个合法的文件描述符, poll时会返回POLLNVAL
            pfd.fd = -channel->fd() - 1;	// 这样子设置是为了removeChannel优化, -1是为了防止0, -0还是0
        }
    }
}

void PollPoller::removeChannel(Channel* channel)
{
    Poller::assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd();
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent());                             // 先调用update将要删除的channel更新为NoneEvent, 即不关注
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
    assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
    size_t n = channels_.erase(channel->fd());                  // 使用key来移除, 返回移除个数; 如果使用迭代器来移除, 返回下一个迭代器
    assert(n == 1); (void)n;
    if (implicit_cast<size_t>(idx) == pollfds_.size() - 1)      // 移除最后一个
    {
        pollfds_.pop_back();
    }
    else
    {
        // 这里移除的算法复杂度是O(1)，将待删除元素与最后一个元素交换再pop_back
        int channelAtEnd = pollfds_.back().fd;
        iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);    // 交换迭代器所指向的元素
        if (channelAtEnd < 0)
        {
            channelAtEnd = -channelAtEnd - 1;                     // 最后一个文件描述符要是不关注为负数, 需修改过来到channels_查找key
        }
        channels_[channelAtEnd]->set_index(idx);                  // 修改交换后的索引
        pollfds_.pop_back();
    }
}