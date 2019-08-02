#include <muduo/net/EventLoop.h>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

/*
    EventLoop在主线程中使用方法, 在子线程中使用方法
*/

void threadFunc()
{
    printf("threadFunc(): pid = %d, tid = %d\n",
        getpid(), CurrentThread::tid());

    EventLoop loop; // 子线程创建EventLoop对象
    loop.loop();    // 在子线程的EventLoop对象中调用loop方法
}

int main(void)
{
    printf("main(): pid = %d, tid = %d\n",
        getpid(), CurrentThread::tid());

    EventLoop loop;
    Thread t(threadFunc);
    t.start();

    loop.loop();    // 主线程即创建loop对象的线程中调用loop方法
    t.join();

    return 0;
}