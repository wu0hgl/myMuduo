#include <muduo/base/Singleton.h>
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

    const muduo::string& name() const { return name_; }
    void setName(const muduo::string& n) { name_ = n; }

private:
    muduo::string name_;
};

void threadFunc()
{
    printf("tid=%d, %p name=%s\n",
           muduo::CurrentThread::tid(),
           &muduo::Singleton<Test>::instance(),
           muduo::Singleton<Test>::instance().name().c_str());
    muduo::Singleton<Test>::instance().setName("only one, changed");    // 这里改变的是主线程中的Test对象, 并没有在构造出一个Test对象
}

int main()
{
    muduo::Singleton<Test>::instance().setName("only one");
    muduo::Thread t1(threadFunc);
    t1.start();
    t1.join();
    printf("tid=%d, %p name=%s\n",
           muduo::CurrentThread::tid(),
           &muduo::Singleton<Test>::instance(),
           muduo::Singleton<Test>::instance().name().c_str());
}