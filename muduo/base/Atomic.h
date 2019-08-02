﻿#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include "muduo/base/noncopyable.h"

#include <stdint.h>

namespace muduo
{
namespace detail
{
template<typename T>
class AtomicIntergerT : noncopyable
{
public:
    AtomicIntergerT() : value_(0)
    {}

    // uncomment if you need copying and assignment
    //  
    // AtomicIntegerT(const AtomicIntegerT& that)
    //   : value_(that.get())
    // {}
    //  
    // AtomicIntegerT& operator=(const AtomicIntegerT& that)
    // {
    //   getAndSet(that.get());
    //   return *this;
    // }

    T get()
    {
        // in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
        return __sync_val_compare_and_swap(&value_, 0, 0);
    }

    T getAndAdd(T x)        // 先获取后加
    {
        // in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
        return __sync_fetch_and_add(&value_, x);
    }

    T addAndGet(T x)        // 先加后获取
    {
        return getAndAdd(x) + x;    //  先获取加之前的值(内部已经加x), 再加x返回出去
    }

    T incrementAndGet()     // 自增1操作
    {
        return addAndGet(1);
    }

    T decrementAndGet()     // 自减1操作
    {
        return addAndGet(-1);
    }

    void add(T x)
    {
        getAndAdd(x);
    }

    void increment()
    {
        incrementAndGet();
    }

    void decrement()
    {
        decrementAndGet();
    }

    T getAndSet(T newValue)
    {
        // in gcc >= 4.7: __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST)
        return __sync_lock_test_and_set(&value_, newValue);
    }

private:
    volatile T value_;
};

}       // namespace detail

typedef detail::AtomicIntergerT<int32_t> AtomicInt32;
typedef detail::AtomicIntergerT<int64_t> AtomicInt64;

}       // namespace muduo

#endif  // MUDUO_BASE_ATOMIC_H