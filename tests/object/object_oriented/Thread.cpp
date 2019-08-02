#include "Thread.h"
#include <iostream>
using namespace std;

Thread::Thread() : autoDelete_(false)
{
    cout << "Thread ..." << endl;
}

Thread::~Thread()
{
    cout << "~Thread ..." << endl;
}

void Thread::Start() 
{
    /* 不可以直接使用Run函数作为线程的函数  
     * Run是一个普通的成员函数, 隐含的第一个参数this指针
     * 而pthread_create是一个普通的C函数指针
     */
    //pthread_create(&threadId_, nullptr, Run, this);

    pthread_create(&threadId_, NULL, ThreadRoutine, this);
}

void Thread::Join() 
{
    pthread_join(threadId_, NULL);
}

void* Thread::ThreadRoutine(void *arg) 
{
    Thread *thread = static_cast<Thread*>(arg);
    thread->Run();              // 使用了虚函数的多态
    if (thread->autoDelete_) 
    {
        delete thread;
    }
    return NULL;                // 具有回调的功能
}

void Thread::setAutoDelete(bool autoDelete) 
{
    autoDelete_ = autoDelete;
}
