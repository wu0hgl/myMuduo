#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{

class Thread : boost::noncopyable
{
public:
    typedef boost::function<void()> ThreadFunc; // 使用基于对象的概念, 定义回调函数类型

    explicit Thread(const ThreadFunc&, const string &name = string());  // 默认空字符
    ~Thread();

    void start();   // 启动线程
    int join();     // 回收线程

    bool started() const { return started_; }
    pthread_t pthreadId() const { return pthreadId_; }
    pid_t tid() const { return tid_; }
    const string& name() const { return name_; }

    static int numCreated() { return numCreated_.get(); }   // 启动线程的个数

private:
    static void *startThread(void *thread); // pthread_creat中回调函数没有this指针, 故使用静态成员函数, 里面调用runInThread
    void runInThread();                     // 执行多线程函数, 内部设置CurrentThread属性

    bool        started_;
    pthread_t   pthreadId_;
    pid_t       tid_;
    ThreadFunc  func_;
    string      name_;

    static AtomicInt32  numCreated_;    // 原子操作, 记录线程的个数
};

}       // namespace muduo

#endif  // MUDUO_BASE_THREAD_H