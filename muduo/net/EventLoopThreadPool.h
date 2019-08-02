#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace muduo
{

namespace net
{

class EventLoop;
class EventLoopThread;
    
class EventLoopThreadPool : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop* baseLoop);
    ~EventLoopThreadPool();
    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());
    EventLoop* getNextLoop();

private:
    EventLoop* baseLoop_;   // 与Acceptor所属EventLoop相同
    bool started_;          // 是否启动
    int numThreads_;        // 线程数, 0为单线程模式
    int next_;              // 新连接到来，所选择的EventLoop对象下标, 一旦选择EventLoop对象, 就是选择对应线程来处理新到来连接
    boost::ptr_vector<EventLoopThread> threads_;    // IO线程列表, 当ptr_vector对象销毁后, 它所管理的EventLoopThread对象一起销毁
    std::vector<EventLoop*> loops_;                 // EventLoop列表, 一个IO线程对应一个EventLoop对象, 这些对象都是栈上对象不需要由EventLoopThreadPool销毁, 因而这里不需要ptr_vector
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_EVENTLOOPTHREADPOOL_H