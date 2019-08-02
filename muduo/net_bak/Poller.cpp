#include <muduo/net/Poller.h>

using namespace muduo;
using namespace muduo::net;

Poller::Poller(EventLoop* loop)
    : ownerLoop_(loop)
{
}

Poller::~Poller()
{
}