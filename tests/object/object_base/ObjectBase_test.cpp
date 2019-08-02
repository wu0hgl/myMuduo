#include "Thread.h"
#include <unistd.h>
#include <iostream>
#include <boost/bind.hpp>
using namespace std;

/*
 * 基于对象编程
 * */

class Foo
{
public:
    Foo(int count) : count_(count) 
    {
    }

    void MemberFunc()          
    {
        while (count_--) 
        {
            cout << "this is a test ... " << count_<< endl;
            sleep(1);
        }
    }

    void MemberFunc2(int x)          
    {
        while (count_--) 
        {
            cout << "x = " << x << " this is a test2 ... " << count_<< endl;
            sleep(1);
        }
    }
    int count_;
};

void ThreadFunc_1() 
{
    cout << "ThreadFunc ..." << endl;
}

void ThreadFunc_2(int count) 
{
    while (count--) 
    {
        cout << "ThreadFunc_2 ..." << endl;
        sleep(1);
    }
}

int main() {
    Thread t1(ThreadFunc_1);
    Thread t2(boost::bind(ThreadFunc_2, 3));
    Foo foo(3);
    Thread t3(boost::bind(&Foo::MemberFunc, foo));
    Foo foo2(3);
    Thread t4(boost::bind(&Foo::MemberFunc2, foo2, 1000));
    

    t1.Start();
    t2.Start();
    t3.Start();
    t4.Start();

    t1.Join();
    t2.Join();
    t3.Join();
    t4.Join();

    return 0;
}
// 
