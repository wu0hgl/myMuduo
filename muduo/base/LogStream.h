#ifndef MUDUO_BASE_LOGSTREAM_H
#define MUDUO_BASE_LOGSTREAM_H

#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <assert.h>
#include <string.h> // memcpy
#ifndef MUDUO_STD_STRING
#include <string>
#endif
#include <boost/noncopyable.hpp>

namespace muduo
{

namespace detail
{

const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

// SIZE为非类型参数  
template<int SIZE>
class FixedBuffer : boost::noncopyable
{
public:
    FixedBuffer()
        : cur_(data_)
    {
        setCookie(cookieStart);             // 并没有调用函数
    }

    ~FixedBuffer()
    {
        setCookie(cookieEnd);
    }

    void append(const char* /*restrict*/ buf, size_t len)           // 添加数据, 实际是拷贝字符串
    {
        // FIXME: append partially                                  // 空间不够用时添加数据函数没有实现
        if (implicit_cast<size_t>(avail()) > len)
        {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }                      // 返回字符数组首地址
    int length() const { return static_cast<int>(cur_ - data_); }   // 已持有数据个数

    // write to data_ directly
    char* current() { return cur_; }                                // 返回cur_指针
    int avail() const { return static_cast<int>(end() - cur_); }    // 当前可用空间
    void add(size_t len) { cur_ += len; }                           // 更改cur_指针位置

    void reset() { cur_ = data_; }                                  // 空间重置, 只需要重置cur_指针, 不需要清空数据, 放数据时直接覆盖
    void bzero() { ::bzero(data_, sizeof data_); }                  // 当前字符数组清0, 相当于::memset(data_, sizeof(data_));

    // for used by GDB
    const char* debugString();                                      // 添加'\0'
    void setCookie(void(*cookie)()) { cookie_ = cookie; }
    // for used by unit test
    string asString() const { return string(data_, length()); }     // 构造一个string对象

private:
    const char* end() const { return data_ + sizeof data_; }        // 缓冲区后一个位置
    // Must be outline function for cookies.
    static void cookieStart();
    static void cookieEnd();    

    void(*cookie_)();   // 函数指针
    char data_[SIZE];   // 通过模板传进缓冲区容量
    char* cur_;         // 当前指针
};
//  |x|x|x|x|x|-|-|-|-|-|
//  ↑	      ↑	    ↑
// date_     cur_       end_最后一个下一个位置

}   // namespace detail

class LogStream : boost::noncopyable
{
    typedef LogStream self;
public:
    typedef detail::FixedBuffer<detail::kSmallBuffer> Buffer;         // 包含上面的缓冲区

    self& operator<<(bool v)
    {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }

    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);

    self& operator<<(const void*);  // 指针转换成十六进制地址存放进去

    self& operator<<(float v)
    {
        *this << static_cast<double>(v);
        return *this;
    }
    self& operator<<(double);
    // self& operator<<(long double);

    self& operator<<(char v)
    {
        buffer_.append(&v, 1);
        return *this;
    }

    // self& operator<<(signed char);
    // self& operator<<(unsigned char);

    self& operator<<(const char* v)
    {
        buffer_.append(v, strlen(v));
        return *this;
    }

    self& operator<<(const string& v)
    {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }

#ifndef MUDUO_STD_STRING
    self& operator<<(const std::string& v)
    {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }
#endif

    self& operator<<(const StringPiece& v)
    {
        buffer_.append(v.data(), v.size());
        return *this;
    }

    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    void staticCheck();

    template<typename T>        // 成员模板, 用于转换
    void formatInteger(T);

    Buffer buffer_;

    static const int kMaxNumericSize = 32;
};

class Fmt // : boost::noncopyable
{
public:
    template<typename T>            // 成员模板, T为算术类型
    Fmt(const char* fmt, T val);    // 整数按照fmt形式格式化

    const char* data() const { return buf_; }
    int length() const { return length_; }

private:
    char buf_[32];
    int length_;
};

inline LogStream& operator<<(LogStream& s, const Fmt& fmt)
{
    s.append(fmt.data(), fmt.length());
    return s;
}

}       // namespace muduo

#endif  // MUDUO_BASE_LOGSTREAM_H