#ifndef MUDUO_BASE_EXCEPTION_H
#define MUDUO_BASE_EXCEPTION_H

#include <muduo/base/Types.h>
#include <exception>

namespace muduo
{

class Exception : public std::exception
{
public:
    explicit Exception(const char* what);
    explicit Exception(const string& what);
    virtual ~Exception() throw();

    /* 异常使用C风格字符串 */
    virtual const char* what() const throw();
    const char* stackTrace() const throw();

private:
    void fillStackTrace();
    string demangle(const char* symbol);    // 名称转换函数

    string message_;    // 异常打印信息
    string stack_;      // 异常调用栈
};

}       // namespace muduo

#endif  // MUDUO_BASE_EXCEPTION_H