#include "Thread.h"
#include <unistd.h>
#include <iostream>
using namespace std;

/*
 * 面向对象的编程风格会暴露抽象类
 * */

class TestThread : public Thread 
{
public:
    TestThread(int count) : count_(count) 
    {
        cout << "TestThread ..." << endl;
    }

    ~TestThread()
    {
        cout << "~TestThread ..." << endl;
    }
private:
    void Run()          // 不应该直接调用Run
    {
        while (count_--) 
        {
            cout << "this is a test ... " << count_<< endl;
            sleep(1);
        }
    }
    int count_;
};

int main() {
    /*
    TestThread t(5);
    t.Start();
    //t.Run();      // 若把Run做成public, 直接调用其实是在主线程里执行函数
    t.Join();
    cout << "=================" << endl;
    */
    TestThread *t1 = new TestThread(3);
    t1->setAutoDelete(true);
    t1->Start();
    t1->Join();
    while (1)
    {
        pause();
    }

    return 0;
}
