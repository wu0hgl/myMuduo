#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace muduo
{
template<typename T>
class Singleton : boost::noncopyable
{
public:
    static T& instance()
    {
        pthread_once(&ponce_, &Singleton::init);    // 保证线程只被初始化一次
        return *value_;
    }

private:
    Singleton();    // 单例, 构造与析构函数为私有属性
    ~Singleton();

    static void init()  // 注册程序退出时
    {
        value_ = new T();
        ::atexit(destroy);  
    }

    static void destroy()
    {
        typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
        T_must_be_complete_type dummy;  (void)dummy;    // 否则T_must_be_complete_type未使用报错
        delete value_;
    }

    static pthread_once_t ponce_;
    static T              *value_;
};

template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;    // 初始值应为PTHREAD_ONCE_INIT

template<typename T>
T* Singleton<T>::value_ = NULL;

}       // muduo

#endif  // MUDUO_BASE_SINGLETON_H