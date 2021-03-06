﻿#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

namespace muduo
{

template<typename T>
class ThreadLocalSingleton : boost::noncopyable
{
public:

    /* 不需要按照线程安全的方式实现, 因为每个线程都有一个t_value_指针 */
    static T& instance()    // Singleton线程安全实现方式pthread_once(&ponce_, &Singleton::init); 保证线程只被初始化一次
    {
        if (!t_value_)      // 通过静态成员保证T对象只被初始化一次
        {
            printf("constructing T\n");
            t_value_ = new T();
            deleter_.set(t_value_);
        }
        return *t_value_;
    }

    static T* pointer()
    {
        return t_value_;
    }

private:

    static void destructor(void *obj)
    {
        assert(obj == t_value_);
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        T_must_be_complete_type dummy; (void)dummy;
        delete t_value_;
        t_value_ = 0;
    }

    class Deleter
    {
    public:
        Deleter()
        {
            pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
        }

        ~Deleter()
        {
            pthread_key_delete(pkey_);
        }

        void set(T *newObj)
        {
            assert(pthread_getspecific(pkey_) == NULL);
            pthread_setspecific(pkey_, newObj);
        }

        pthread_key_t pkey_;
    };

    static __thread T   *t_value_;  // 指针是pod类型, 所以线程本地存储可以使用关键字__thread修饰, 每个线程都有一份
    static Deleter      deleter_;   // 用于销毁指针所指向的对象 
};

template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}       // muduo

#endif  // MUDUO_BASE_THREADLOCALSINGLETON_H