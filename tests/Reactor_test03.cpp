#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

#include <stdio.h>
#include <sys/timerfd.h>

using namespace muduo;
using namespace muduo::net;

/*
    使用timerfd_create定时调用回调函数, Poller(EPollPoller)采用电平触发, 所以一定要把数据从内核缓冲区读走, 否则一直触发, 并不代表定时器触发条件满足一直触发
*/

EventLoop* g_loop;
int timerfd;

void timeout(Timestamp receiveTime)
{
    printf("Timeout!\n");
    uint64_t howmany;
    ::read(timerfd, &howmany, sizeof howmany);  // 把数据从内核缓冲区读走
    g_loop->quit();                             // 退出loop, io线程中调用quit函数
}

int main(void)
{
    EventLoop loop;
    g_loop = &loop;

    timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    Channel channel(&loop, timerfd);
    channel.setReadCallback(boost::bind(timeout, _1));  // 设置回调函数
    channel.enableReading();                            // Channel关注可读事件

    struct itimerspec howlong;
    bzero(&howlong, sizeof howlong);                    // it_interval清零, 一次性定时器
    howlong.it_value.tv_sec = 1;
    ::timerfd_settime(timerfd, 0, &howlong, NULL);

    loop.loop();

    ::close(timerfd);
}
