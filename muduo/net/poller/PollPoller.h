#ifndef MUDUO_NET_POLLER_POLLPOLLER_H
#define MUDUO_NET_POLLER_POLLPOLLER_H

#include <muduo/net/Poller.h>

#include <map>
#include <vector>

struct pollfd;

namespace muduo
{

namespace net
{

///
/// IO Multiplexing with poll(2).
///
class PollPoller : public Poller
{
public:
    /* ChannelList类型定义基类在Poller中 */
    PollPoller(EventLoop* loop);
    virtual ~PollPoller();

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels); // activeChannels传出参数, 返回触发的Channel数组
    virtual void updateChannel(Channel* channel);                       // 更新对应Channel的事件, 包括注册Channel
    virtual void removeChannel(Channel* channel);                       // 从Poller中移除对应的Channel

private:
    /* poll调用此函数, 向activeChannels列表填充响应事件, 并设置相应Channel的revents_(用于指示Channel处理什么事件) */
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    typedef std::vector<struct pollfd> PollFdList;
    typedef std::map<int, Channel*> ChannelMap;	    // key是文件描述符，value是Channel*

    PollFdList pollfds_;
    ChannelMap channels_;
};

}       // namespace net
        
}       // namespace muduo

#endif  // MUDUO_NET_POLLER_POLLPOLLER_H