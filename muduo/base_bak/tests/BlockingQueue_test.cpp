#include <muduo/base/BlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

/*
    Test::run -> 生产者线程, 等待消费者执行时才开始生产产品
    Treas::threadFunc -> 消费者线程
*/

class Test
{
public:
    Test(int numThreads)
        : latch_(numThreads),
        threads_(numThreads)    // 创建线程数量和计数值相同
    {
        for (int i = 0; i < numThreads; ++i)
        {
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);
            threads_.push_back(new muduo::Thread(boost::bind(&Test::threadFunc, this), muduo::string(name)));
        }
        for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::start, _1));
    }

    void run(int times)
    {
        printf("waiting for count down latch\n");
        latch_.wait();      // 五个子线程减把计数latch_减为0之后, 主线程才可以运行
        printf("all threads started\n");
        for (int i = 0; i < times; ++i)
        {
            char buf[32];
            snprintf(buf, sizeof buf, "hello %d", i);
            queue_.put(buf);
            printf("tid=%d, put data = %s, size = %zd\n", muduo::CurrentThread::tid(), buf, queue_.size());
        }
    }

    void joinAll()
    {
        for (size_t i = 0; i < threads_.size(); ++i)
        {
            queue_.put("stop");
        }

        for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::join, _1));
    }

private:

    void threadFunc()
    {
        printf("tid=%d, %s started\n", muduo::CurrentThread::tid(), muduo::CurrentThread::name());

        latch_.countDown();                     // 计数latch_-1
        bool running = true;
        while (running)
        {
            std::string d(queue_.take());
            printf("tid=%d, get data = %s, size = %zd\n", muduo::CurrentThread::tid(), d.c_str(), queue_.size());
            running = (d != "stop");
        }

        printf("tid=%d, %s stopped\n", muduo::CurrentThread::tid(), muduo::CurrentThread::name());
    }

    muduo::BlockingQueue<std::string> queue_;   // 无界缓冲队列
    muduo::CountDownLatch latch_;
    boost::ptr_vector<muduo::Thread> threads_;
};

int main()
{
    printf("pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());
    Test t(5);
    t.run(100);     // 主线程等待5个子线程先创建执行执行后, 再执行run, 此时等待倒计数latch_减为0
    t.joinAll();

    printf("number of created threads %d\n", muduo::Thread::numCreated());
}