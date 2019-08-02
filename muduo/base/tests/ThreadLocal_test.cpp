#include <muduo/base/ThreadLocal.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Thread.h>

#include <boost/noncopyable.hpp>
#include <stdio.h>

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

muduo::ThreadLocal<Test> testObj1;  // 每个线程都有一个这个对象
muduo::ThreadLocal<Test> testObj2;  // 全局对象, 相当于地址

void print()
{
    printf("===========start print=============\n");
    printf("tid=%d, obj1 %p name=%s\n",
        muduo::CurrentThread::tid(),
        &testObj1.value(),
        testObj1.value().name().c_str());
    printf("tid=%d, obj2 %p name=%s\n",
        muduo::CurrentThread::tid(),
        &testObj2.value(),
        testObj2.value().name().c_str());
    printf("===========end print=============\n");
}

void threadFunc()
{
    print();
    testObj1.value().setName("changed 1");
    testObj2.value().setName("changed 42");
    print();
}

int main()
{
    testObj1.value().setName("main one");
    print();
    muduo::Thread t1(threadFunc);
    t1.start();
    t1.join();
    testObj2.value().setName("main two");
    print();

    pthread_exit(0);
}

/* 主线程与t1执行顺序不确定, 访问线程私有的Test类都是通过相同的key(全局变量): testObj1, testObj2来实现的, 相当于一个同名而不同值(在每个线程中)的全局变量, 一键多值
constructing ThreadLocal                   --> 在ThreadLocal类的构造函数中加的打印信息, 全局变量
constructing ThreadLocal                   --> 在程序的一开始就构造出两个ThreadLocal<Test>, 相当于每个线程通过相同key访问自己私有的value
tid=6972, constructing 0xf37030            --> 55行testObj1.value()会构造一个Test对象
===========start print=============
tid=6972, obj1 0xf37030 name=main one      --> Test对象在55行已经构造出来了
tid=6972, constructing 0xf37060            --> 40行构造Test对象
tid=6972, obj2 0xf37060 name=              --> 默认线程名称为空string
===========end print=============
===========start print=============        --> 47行, 在子线程中, 执行print函数会构造出两个对象
tid=6973, constructing 0x7f3dc40008c0      
tid=6973, obj1 0x7f3dc40008c0 name=
tid=6973, constructing 0x7f3dc40008f0
tid=6973, obj2 0x7f3dc40008f0 name=
===========end print=============
===========start print=============        --> 50行打印48行与49行改变名称
tid=6973, obj1 0x7f3dc40008c0 name=changed 1    
tid=6973, obj2 0x7f3dc40008f0 name=changed 42
===========end print=============         
tid=6973, destructing 0x7f3dc40008c0 changed 1  --> 59: t1.join回收子线程, 同时析构两个子进程的Test对象
tid=6973, destructing 0x7f3dc40008f0 changed 42
===========start print=============             --> 61行执打印操作, 此时主线程中两个Test对象都有名称
tid=6972, obj1 0xf37030 name=main one
tid=6972, obj2 0xf37060 name=main two
===========end print=============
tid=6972, destructing 0xf37030 main one         --> 程序执行结束, 析构主进程的两个Test对象
tid=6972, destructing 0xf37060 main two
*/