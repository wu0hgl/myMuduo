#include <muduo/base/ThreadPool.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/CurrentThread.h>

#include <boost/bind.hpp>
#include <stdio.h>

void print()
{
    printf("tid=%d\n", muduo::CurrentThread::tid());
}

void printString(const std::string& str)
{
    printf("tid=%d, str=%s\n", muduo::CurrentThread::tid(), str.c_str());
}

int main()
{
    muduo::ThreadPool pool("MainThreadPool");
    pool.start(5);

    pool.run(print);
    pool.run(print);
    for (int i = 0; i < 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.run(boost::bind(printString, std::string(buf)));   // 添加任务, 这里传参时std::string(buf), 不是栈上对象
    }

    /*
    muduo::CountDownLatch latch(2);
    pool.run(boost::bind(&muduo::CountDownLatch::countDown, &latch));   // 倒计数2, 只有一个muduo::CountDownLatch::countDown把计数减为1, 应该再添加一个muduo::CountDownLatch::countDown
    latch.wait();               // 此时倒计数为1, 阻塞等待
    pool.stop();
    */
    muduo::CountDownLatch latch(1);
    pool.run(boost::bind(&muduo::CountDownLatch::countDown, &latch));
    latch.wait();
    pool.stop();
}