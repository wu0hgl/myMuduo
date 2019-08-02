#ifndef __THREAD_H__
#define __THREAD_H__
#include <pthread.h>

class Thread 
{
public:
    Thread();
    virtual ~Thread();  // 线程类是一个多态基类, 析构函数需是虚函数
    void Start();
    void Join();        
    void setAutoDelete(bool autoDelete);
private:
    virtual void Run() = 0;     // 使不同线程的执行体有不同的执行体, 外界不能直接访问它
    static void *ThreadRoutine(void *arg);
    pthread_t   threadId_;
    bool        autoDelete_;
};

#endif  // __THREAD_H__
