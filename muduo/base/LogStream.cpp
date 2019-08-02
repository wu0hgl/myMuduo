#include <muduo/base/LogStream.h>

#include <algorithm>
#include <limits>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::detail;

#pragma GCC diagnostic ignored "-Wtype-limits"
//#pragma GCC diagnostic error "-Wtype-limits"

namespace muduo
{

namespace detail
{

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;              // 指向digits+9位置, 即0位置
BOOST_STATIC_ASSERT(sizeof(digits) == 20);

const char digitsHex[] = "0123456789ABCDEF";
BOOST_STATIC_ASSERT(sizeof digitsHex == 17);

// Efficient Integer to String Conversions, by Matthew Wilson.
template<typename T>
size_t convert(char buf[], T value)         // 通过digits指向位置来转换字符串
{
    T i = value;
    char* p = buf;  // 指针指向buf

    do
    {
        int lsd = static_cast<int>(i % 10);                             // 得到最后一个数字 
        i /= 10;
        *p++ = zero[lsd];                                               // zero指针指向0位置
    } while (i != 0);

    if (value < 0)
    {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(buf, p);                                             // 负数字符串逆转, 左闭右开逆序规则, 即p指向的'\0'字符不被包含在逆序的范围内

    return p - buf;                                                   // 转换字符数
}

// uintptr_t对于32位平台来说就是unsigned int
// 对于64位平台来说是unsigned long int
size_t convertHex(char buf[], uintptr_t value)  // 通过digitsHex指向位置来转换字符串
{
    uintptr_t i = value;
    char* p = buf;

    do
    {
        int lsd = i % 16;
        i /= 16;
        *p++ = digitsHex[lsd];
    } while (i != 0);

    *p = '\0';
    std::reverse(buf, p);

    return p - buf;
}

}   // namespace detail

}   // namespace muduo

template<int SIZE>
const char* FixedBuffer<SIZE>::debugString()
{
    *cur_ = '\0';
    return data_;
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieStart()
{
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieEnd()
{
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

void LogStream::staticCheck()
{
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10);
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10);
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10);
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10);
}

template<typename T>
void LogStream::formatInteger(T v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        size_t len = convert(buffer_.current(), v); // 整数转换为字符串存放进buffer_中
        buffer_.add(len);                           // 调整buffer_中cur_指针
    }
}

LogStream& LogStream::operator<<(short v)
{
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(const void* p)
{
    uintptr_t v = reinterpret_cast<uintptr_t>(p);                     // 指针强制转换uintptr_t
    if (buffer_.avail() >= kMaxNumericSize)
    {
        char* buf = buffer_.current();
        buf[0] = '0';
        buf[1] = 'x';
        size_t len = convertHex(buf + 2, v);
        buffer_.add(len + 2);
    }
    return *this;
}

// FIXME: replace this with Grisu3 by Florian Loitsch.
LogStream& LogStream::operator<<(double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }
    return *this;
}

template<typename T>
Fmt::Fmt(const char* fmt, T val)
{
    // 断言T是算术类型
    BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value == true);

    length_ = snprintf(buf_, sizeof buf_, fmt, val);
    assert(static_cast<size_t>(length_) < sizeof buf_);
}

// Explicit instantiations, 显式实例化
// 类型特化
template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);