#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace muduo
{

class ThreadPool : boost::noncopyable
{
public:
    typedef boost::function<void()> Task;

    explicit ThreadPool(const string &name = string());
    ~ThreadPool();

    void start(int numThreads); // numThread: 线程池内线程数量, 构造启动线程  
    void stop();                // 调用muduo::Thread::join回收线程

    void run(const Task &f);    // 生产者, 向queue_添加任务, 若threads_为空, 即单线线程, 直接执行Task

private:
    void runInThread();         // 调用take后, 执行任务
    Task take();                // 消费者, 从queue_获取任务, 任务队列为空时等待条件变量cond_

    MutexLock   mutex_;         // 配合条件变量使用的互斥量
    Condition   cond_;          // 任务队列不为空条件变量
    string      name_;          // 线程名默认空字符串
    boost::ptr_vector<muduo::Thread> threads_;  // 这里需要加muduo域?
    std::deque<Task> queue_;
    bool        running_;
};

}       // muduo

#endif  // MUDUO_BASE_THREADPOOL_H