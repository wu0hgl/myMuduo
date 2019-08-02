﻿#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

using namespace muduo;

/*
    倒计数门栓量为1, 主线程执行t.run()后, 子线程开始执行, 在此之前子线程阻塞在条件变量处
*/

class Test
{
public:
    Test(int numThreads)
        : latch_(1),
        threads_(numThreads)
    {
        for (int i = 0; i < numThreads; ++i)
        {
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);
            threads_.push_back(new muduo::Thread(boost::bind(&Test::threadFunc, this), muduo::string(name)));
        }
        for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::start, _1));    // 这里_1表示vector的遍历对象, &Thread::start需要对象
    }

    void run()
    { latch_.countDown(); }

    void joinAll()
    { for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1)); }

private:

    void threadFunc()
    {
        latch_.wait();
        printf("tid=%d, %s started\n", CurrentThread::tid(), CurrentThread::name());
        printf("tid=%d, %s stopped\n", CurrentThread::tid(), CurrentThread::name());
    }

    CountDownLatch latch_;
    boost::ptr_vector<Thread> threads_;
};

int main()
{
    printf("pid=%d, tid=%d\n", ::getpid(), CurrentThread::tid());
    Test t(3);
    sleep(3);
    printf("pid=%d, tid=%d %s running ...\n", ::getpid(), CurrentThread::tid(), CurrentThread::name());
    t.run();    // 发起起跑命令
    t.joinAll();

    printf("number of created threads %d\n", Thread::numCreated());
}