#include <muduo/base/CountDownLatch.h>  // 程序中没有用到
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <vector>
#include <stdio.h>

using namespace muduo;
using namespace std;

MutexLock g_mutex;      // 全局锁
vector<int> g_vec;
const int kCount = 10 * 1000 * 1000;

void threadFunc()
{
    for (int i = 0; i < kCount; ++i)
    {
        MutexLockGuard lock(g_mutex);
        g_vec.push_back(i);
    }
}

int main()
{
    const int kMaxThreads = 8;
    g_vec.reserve(kMaxThreads * kCount);

    Timestamp start(Timestamp::now());
    for (int i = 0; i < kCount; ++i)
    {
        g_vec.push_back(i);
    }

    printf("single thread without lock %f\n", timeDifference(Timestamp::now(), start));

    start = Timestamp::now();
    threadFunc();
    printf("single thread with lock %f\n", timeDifference(Timestamp::now(), start));

    for (int nthreads = 1; nthreads < kMaxThreads; ++nthreads)  // 依次创建1..7个线程查数
    {
        boost::ptr_vector<Thread> threads;
        g_vec.clear();
        start = Timestamp::now();
        for (int i = 0; i < nthreads; ++i)
        {
            threads.push_back(new Thread(&threadFunc));
            threads.back().start();
        }
        for (int i = 0; i < nthreads; ++i)
        {
            //printf("retrieve thread: %d\n", i); 按顺序回收线程
            threads[i].join();
        }
        printf("%d thread(s) with lock %f\n", nthreads, timeDifference(Timestamp::now(), start));
    }
}

