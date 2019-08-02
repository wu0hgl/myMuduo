#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H

#include <muduo/base/copyable.h>    // 为了实现重载<操作符
#include <muduo/base/Types.h>

#include <boost/operators.hpp>

namespace muduo 
{
class Timestamp : public muduo::copyable,                       // 空基类, 标识类, 值类型, 可拷贝
                  public boost::less_than_comparable<Timestamp> // 继承boost::less_than_comparable<Timestamp>要求实现<操作符, 之后可自动实现>, <=, >=(根据<自动推导)
{
public:
    Timestamp() : microSecondsSinceEpoch_(0) {}

    explicit Timestamp(int64_t microSecondsSinceEpoch);

    // default copy/assignment/dtor are Okay

    void swap(Timestamp &that)          // 交换时间
    {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);   // 交换时间
    }

    string toString() const;            // 打印秒数, 有小数
    string toFormattedString() const;   // 固定格式打印

    bool valid() const          // 时间是否有效
    {
        return microSecondsSinceEpoch_ > 0;
    }

    int64_t microSecondsSinceEpoch() const  // 返回成员变量
    { 
        return microSecondsSinceEpoch_; 
    }

    static Timestamp now();     // 通过静态成员变量构造对象, 然后通过拷贝构造函数进行初始化

    static Timestamp invalid();

    static const int kMicroSecondsPerSecond = 1000 * 1000;          // 微秒到秒的转换

private:
    int64_t microSecondsSinceEpoch_;                                // 微秒, 距离1970年零时的时间
};

inline bool operator<(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs)
{
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline double timeDifference(Timestamp high, Timestamp low) 
{
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;       // 除以微秒转换成秒的单位
}

inline Timestamp addTime(Timestamp timestamp, double second)
{
    int64_t delta = static_cast<int>(second * Timestamp::kMicroSecondsPerSecond);   // 秒转换成微秒
    return Timestamp(delta + timestamp.microSecondsSinceEpoch());
}

}       // namespace muduo

#endif  // MUDUO_BASE_TIMESTAMP_H
