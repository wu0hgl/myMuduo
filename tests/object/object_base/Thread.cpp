#include "Thread.h"
#include <iostream>
using namespace std;

Thread::Thread(const ThreadFunc &func) : func_(func), autoDelete_(false) {
    //cout << "Thread ..." << endl;
}

void Thread::Start() 
{
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

void Thread::Run() 
{
    func_();
}
