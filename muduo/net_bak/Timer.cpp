#include <muduo/net/Timer.h>

using namespace muduo;
using namespace muduo::net;

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
    if (repeat_)    // 重复定时器
    {   
        // 重新计算下一个超时时刻, 当前时间+重复时间间隔
        expiration_ = addTime(now, interval_);  // addTime在Timestamp.h中定义, 这里使用值传递而不是引用传递, Timestamp只有一个int64_t类型变量, 会把参数传递到8字节寄存器中, 而不是传递到堆栈中, 效率更高些
    }
    else            // 不是重复时间定时器, 超时时间为非法时间
    {
        expiration_ = Timestamp::invalid();
    }
}