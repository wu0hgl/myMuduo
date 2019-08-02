#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <stdio.h>

/*
    每个线程各自有一个Test单例对象
*/

class Test : boost::noncopyable
{
public:
    Test()
    {
        printf("tid=%d, constructing %p\n", muduo::CurrentThread::tid(), this);
    }

    ~Test()
    {
        printf("tid=%d, destructing %p %s\n", muduo::CurrentThread::tid(), this, name_.c_str());
    }

    const std::string& name() const { return name_; }
    void setName(const std::string& n) { name_ = n; }

private:
    std::string name_;
};

void threadFunc(const char* changeTo)
{
    printf("===========%s start print=============\n", changeTo);
    printf("tid=%d, %p name=%s\n",
           muduo::CurrentThread::tid(),
           &muduo::ThreadLocalSingleton<Test>::instance(),
           muduo::ThreadLocalSingleton<Test>::instance().name().c_str());

    muduo::ThreadLocalSingleton<Test>::instance().setName(changeTo);
    sleep(1);       // 不加睡眠, 线程t1与t2构造Test对象使用相同的地址空间
    printf("tid=%d, %p name=%s\n",
           muduo::CurrentThread::tid(),
           &muduo::ThreadLocalSingleton<Test>::instance(),
           muduo::ThreadLocalSingleton<Test>::instance().name().c_str());
    printf("===========%s end print=============\n", changeTo);
    // no need to manually delete it
    // muduo::ThreadLocalSingleton<Test>::destroy();
}

int main()
{
    /* 每个线程都有一个单例对象 */
    muduo::ThreadLocalSingleton<Test>::instance().setName("main one"); // SingletonThreadLocal_test是每个线程共享一个单例对象
    muduo::Thread t1(boost::bind(threadFunc, "thread1"));
    muduo::Thread t2(boost::bind(threadFunc, "thread2"));
    t1.start();
    t2.start();
    t1.join();
    printf("tid=%d, %p name=%s\n",
           muduo::CurrentThread::tid(),
           &muduo::ThreadLocalSingleton<Test>::instance(),
           muduo::ThreadLocalSingleton<Test>::instance().name().c_str());
    t2.join();

    pthread_exit(0);
}

/* t1与t2线程执行顺序不确定, T构造了三次
constructing T
tid=9410, constructing 0x1cb5030
===========thread2 start print=============
constructing T
tid=9412, constructing 0x7f2c180008c0
tid=9412, 0x7f2c180008c0 name=
===========thread1 start print=============
constructing T
tid=9411, constructing 0x7f2c100008c0
tid=9411, 0x7f2c100008c0 name=
tid=9412, 0x7f2c180008c0 name=thread2
===========thread2 end print=============
tid=9412, destructing 0x7f2c180008c0 thread2
tid=9411, 0x7f2c100008c0 name=thread1
===========thread1 end print=============
tid=9411, destructing 0x7f2c100008c0 thread1
tid=9410, 0x1cb5030 name=main one
tid=9410, destructing 0x1cb5030 main one
*/