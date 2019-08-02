#include <muduo/net/EventLoop.h>
//#include <muduo/net/EventLoopThread.h>
//#include <muduo/base/Thread.h>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

EventLoop* g_loop;
int g_flag = 0;

void run4()
{
    printf("run4(): pid = %d, flag = %d\n", getpid(), g_flag);
    g_loop->quit();
}

void run3()                     
{
    printf("run3(): pid = %d, flag = %d\n", getpid(), g_flag);  // run1执行后会执行run3, 这时flag=2
    g_loop->runAfter(3, run4);
    g_flag = 3;
}

void run2()
{
    printf("run2(): pid = %d, flag = %d\n", getpid(), g_flag);
    g_loop->queueInLoop(run3);  // 回调任务添加到队列当中
}

void run1()
{
    g_flag = 1;
    printf("run1(): pid = %d, flag = %d\n", getpid(), g_flag);
    g_loop->runInLoop(run2);    // 单线程程序, 调用run1还是在当前线程中, 不会放在队列中, 而是直接执行run2
    g_flag = 2;
}

int main()
{
    printf("main(): pid = %d, flag = %d\n", getpid(), g_flag);

    EventLoop loop;
    g_loop = &loop;

    loop.runAfter(2, run1);
    loop.loop();
    printf("main(): pid = %d, flag = %d\n", getpid(), g_flag);
}