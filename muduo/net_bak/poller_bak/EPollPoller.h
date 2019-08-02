#ifndef MUDUO_NET_POLLER_EPOLLPOLLER_H
#define MUDUO_NET_POLLER_EPOLLPOLLER_H

#include <muduo/net/Poller.h>

#include <map>
#include <vector>

struct epoll_event;

namespace muduo
{

namespace net
{

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    virtual ~EPollPoller();

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
    virtual void updateChannel(Channel* channel);
    virtual void removeChannel(Channel* channel);

private:
    static const int kInitEventListSize = 16;

    void fillActiveChannels(int numEvents,
        ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    typedef std::vector<struct epoll_event> EventList;
    typedef std::map<int, Channel*> ChannelMap;

    int epollfd_;
    EventList events_;          // 初始值为kInitEventListSize
    ChannelMap channels_;
};

}       // namespace muduo

}       // namespace muduo

#endif  // MUDUO_NET_POLLER_EPOLLPOLLER_H