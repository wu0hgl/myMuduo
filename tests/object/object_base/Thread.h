#ifndef __THREAD_H__
#define __THREAD_H__
#include <pthread.h>
#include <boost/function.hpp>

class Thread 
{
public:
    typedef boost::function<void ()> ThreadFunc;

    explicit Thread(const ThreadFunc &func);
    void Start();
    void Join();        
    void setAutoDelete(bool autoDelete);
private:
    void Run();     // 使不同线程的执行体有不同的执行体, 外界不能直接访问它
    static void *ThreadRoutine(void *arg);
    ThreadFunc  func_;
    pthread_t   threadId_;
    bool        autoDelete_;
};

#endif  // __THREAD_H__
