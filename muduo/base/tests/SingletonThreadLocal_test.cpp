#include <muduo/base/Singleton.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/ThreadLocal.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <stdio.h>

/*
    是每个线程共享一个Test单例对象
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

// instance对象本身是所有线程所共有的, 
// 但是Singleton里面类型muduo::ThreadLocal<Test>对象里面所持有的test对象是每个线程各自有一份
// value返回每个线程所特定的test对象
#define STL muduo::Singleton<muduo::ThreadLocal<Test> >::instance().value()

void print()
{
    printf("tid=%d, %p name=%s\n",
           muduo::CurrentThread::tid(),
           &STL,
           STL.name().c_str());
}

void threadFunc(const char* changeTo)
{
    printf("===========%s start print=============\n", changeTo);
    print();
    STL.setName(changeTo);
    sleep(1);   // 不加睡眠, 线程t1与t2构造Test对象使用相同的地址空间
    print();
    printf("===========%s end print=============\n", changeTo);
}

int main()
{
    STL.setName("main one");
    muduo::Thread t1(boost::bind(threadFunc, "thread1"));
    muduo::Thread t2(boost::bind(threadFunc, "thread2"));
    t1.start();
    t2.start();
    t1.join();
    print();
    t2.join();
    pthread_exit(0);
}

/*
constructing ThreadLocal
tid=9193, constructing 0x1c37050
===========thread2 start print=============
tid=9195, constructing 0x7f40300008c0
tid=9195, 0x7f40300008c0 name=
===========thread1 start print=============
tid=9194, constructing 0x7f40280008c0
tid=9194, 0x7f40280008c0 name=
tid=9195, 0x7f40300008c0 name=thread2
===========thread2 end print=============
tid=9194, 0x7f40280008c0 name=thread1
===========thread1 end print=============
tid=9194, destructing 0x7f40280008c0 thread1
tid=9193, 0x1c37050 name=main one
tid=9195, destructing 0x7f40300008c0 thread2
tid=9193, destructing 0x1c37050 main one
*/