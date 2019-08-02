#include <muduo/net/EventLoop.h>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

/*
    EventLoop中的loop方法要在创建EventLoop中的线程中调用!!!
*/

EventLoop *g_loop;

void threadFunc()
{
    g_loop->loop();     // 未在创建EventLoop对象的线程中调用loop, 出错
}

int main()
{
    EventLoop loop;

    g_loop = &loop;

    Thread t(threadFunc);
    t.start();
    t.join();

    return 0;
}