#include <muduo/net/EventLoopThread.h>

#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
    : loop_(NULL),
    exiting_(false),
    thread_(boost::bind(&EventLoopThread::threadFunc, this)),
    mutex_(),
    cond_(mutex_),
    callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    loop_->quit();      // 退出IO线程，让IO线程的loop循环退出，从而退出了IO线程
    thread_.join();
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    /* 线程启动后会调用构造函数中绑定的EventLoopThread::threadFunc */
    thread_.start();            // 这时会有两个线程, 调用thread_.start的线程和新创建的线程, 两个线程并发执行
    /* 下面与EventLoopThread::threadFunc执行顺序不确定 */
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL)   // 使用条件变量等待loop_不为空
        {
            cond_.wait();
        }
    } 

    // 这时EventLoopThread::threadFunc中的loop执行起来了, 可以返回EventLoop对象指针
    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    if (callback_)			// 可以在构造函数中传递进来
    {
        callback_(&loop);
    }

  {
      MutexLockGuard lock(mutex_);
      // loop_指针指向了一个栈上的对象，threadFunc函数退出之后，这个指针就失效了
      // threadFunc函数退出，就意味着线程退出了，EventLoopThread对象也就没有存在的价值了。 muduo对于线程类封装并没有实现自动销毁型, 因为线程池中的线程数固定, 开始时创建好, 与整个程序的生存期相同, 也就是就说如果loop.loop对出, 整个程序就退出了, 因此销毁或不销毁EventLoopThread对象都没有关系, 不能通过loop_访问EventLoop对象 
      // 因而不会有什么大的问题
      loop_ = &loop;
      cond_.notify();
  }

  loop.loop();
  //assert(exiting_);
}