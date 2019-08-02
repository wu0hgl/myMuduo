﻿#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H

#include <muduo/base/copyable.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>

#include <muduo/net/Endian.h>

#include <algorithm>
#include <vector>

#include <assert.h>
#include <string.h>
//#include <unistd.h>  // ssize_t

/*
    Buffer本身不是线程安全, 发送会转到TcpConnection中, 一个TcpConnection只属于一个线程, 所以是线程安全的
    同一时刻只有一个线程在操作, 所以不需要加锁
*/

namespace muduo
{

namespace net
{

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer : public muduo::copyable
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    Buffer()
        : buffer_(kCheapPrepend + kInitialSize),
        readerIndex_(kCheapPrepend),
        writerIndex_(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == kInitialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    // default copy-ctor, dtor and assignment are fine

    void swap(Buffer& rhs)                          // 单纯交换readerIndex_与writerIndex_指针位置
    {
        buffer_.swap(rhs.buffer_);
        std::swap(readerIndex_, rhs.readerIndex_);
        std::swap(writerIndex_, rhs.writerIndex_);
    }

    size_t readableBytes() const                    // 返回可读字节数量
    { return writerIndex_ - readerIndex_; }

    size_t writableBytes() const                    // 返回可写字节数量
    { return buffer_.size() - writerIndex_; }

    size_t prependableBytes() const                 // 返回可读位置偏移量
    { return readerIndex_; }

    const char* peek() const                        // 返回可读位置地址
    { return begin() + readerIndex_; }

    const char* findCRLF() const                    // 从peek()处开始查找\r\n
    {
        const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    const char* findCRLF(const char* start) const   // 从指定位置查找\r\n
    {
        assert(peek() <= start);
        assert(start <= beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? NULL : crlf;
    }

    // retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes());
    // the evaluation of two functions are unspecified
    void retrieve(size_t len)                       // 调整readerIndex_位置
    {
        assert(len <= readableBytes());
        if (len < readableBytes())  // 取出部分数据
        {
            readerIndex_ += len;
        }
        else                        // 取出全部数据
        {
            retrieveAll();
        }
    }

    void retrieveUntil(const char* end)             // 取回直到某个位置
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieveInt32()
    { retrieve(sizeof(int32_t)); }

    void retrieveInt16()
    { retrieve(sizeof(int16_t)); }

    void retrieveInt8()
    { retrieve(sizeof(int8_t)); }

    void retrieveAll()                              // readerIndex_与writerIndex_调整至kCheapPrepend, 相当于清空数据
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    string retrieveAllAsString()                    // 把所有数据取回并返回字符串
    {
        return retrieveAsString(readableBytes());;
    }

    string retrieveAsString(size_t len)             // 取出定长数据返回字符串
    {
        assert(len <= readableBytes());
        string result(peek(), len); // 构造一个字符串
        retrieve(len);              // 只是便宜处理
        return result;
    }

    StringPiece toStringPiece() const               // 把全部可读数据返回出去
    {
        return StringPiece(peek(), static_cast<int>(readableBytes()));
    }

    void append(const StringPiece& str)             // 追加数据char*类型数据
    {
        append(str.data(), str.size());
    }

    void append(const char* /*restrict*/ data, size_t len)  // 追加数据char*类型数据
    {
        ensureWritableBytes(len);		// 确保可写的缓存区大于len
        std::copy(data, data + len, beginWrite());
        hasWritten(len);                // 调整写后的writerIndex_位置
    }

    void append(const void* /*restrict*/ data, size_t len)  // 追加void*类型数据
    {
        append(static_cast<const char*>(data), len);
    }

    // 确保缓冲区可写空间>=len，如果不足则扩充
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    char* beginWrite()                              // 返回可写位置地址
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    void hasWritten(size_t len)
    {
        writerIndex_ += len;
    }

    ///
    /// Append int32_t using network endian
    ///
    /* 尾部添加数据并转换字节序 */
    void appendInt32(int32_t x)
    {
        int32_t be32 = sockets::hostToNetwork32(x);     // 转换网络字节序
        append(&be32, sizeof be32);
    }

    void appendInt16(int16_t x)
    {
        int16_t be16 = sockets::hostToNetwork16(x);
        append(&be16, sizeof be16);
    }

    void appendInt8(int8_t x)                           // 一个字节不需要转化
    {
        append(&x, sizeof x);
    }
    
    ///
    /// Read int32_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    /* 负责从头部读取数同时完成字节序转换, 并偏移对应长度readerIndex_ */
    int32_t readInt32()
    {
        int32_t result = peekInt32();
        retrieveInt32();		// 读的字节调整
        return result;
    }

    int16_t readInt16()
    {
        int16_t result = peekInt16();
        retrieveInt16();
        return result;
    }

    int8_t readInt8()
    {
        int8_t result = peekInt8();
        retrieveInt8();
        return result;
    }

    ///
    /// Peek int32_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    /* 负责从头部读取数同时完成字节序转换, 但不负责readerIndex_偏移 */
    int32_t peekInt32() const
    {
        assert(readableBytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, peek(), sizeof be32);	// 拷贝到be32中
        return sockets::networkToHost32(be32);	// 转换主机字节序
    }

    int16_t peekInt16() const
    {
        assert(readableBytes() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, peek(), sizeof be16);
        return sockets::networkToHost16(be16);
    }

    int8_t peekInt8() const
    {
        assert(readableBytes() >= sizeof(int8_t));
        int8_t x = *peek();
        return x;
    }

    ///
    /// Prepend int32_t using network endian
    ///
    /* 头部添加数据 */
    void prependInt32(int32_t x)
    {
        int32_t be32 = sockets::hostToNetwork32(x);
        prepend(&be32, sizeof be32);
    }

    void prependInt16(int16_t x)
    {
        int16_t be16 = sockets::hostToNetwork16(x);
        prepend(&be16, sizeof be16);
    }

    void prependInt8(int8_t x)
    {
        prepend(&x, sizeof x);
    }

    void prepend(const void* /*restrict*/ data, size_t len)
    {
        assert(len <= prependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d + len, begin() + readerIndex_);
    }

    // 收缩，保留reserve个字节
    void shrink(size_t reserve)                         // 从套接字fd读取数据并添加至缓冲区中
    {
        // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
        Buffer other;
        other.ensureWritableBytes(readableBytes() + reserve);
        other.append(toStringPiece());
        swap(other);
    }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t readFd(int fd, int* savedErrno);    // 从套接字fd读取数据并添加至缓冲区中

private:

    char* begin()                                       // 返回buffer_首地址
    {
        return &*buffer_.begin();
    }

    const char* begin() const                           // 返回buffer_首地址
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)                          // 空间不足扩充数组, 空间满足搬移数据
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) // 空间不足
        {
            // FIXME: move readable data
            buffer_.resize(writerIndex_ + len);
        }
        else                                                            // 空间满足, 数据搬移
        {
            // move readable data to the front, make space inside buffer
            assert(kCheapPrepend < readerIndex_);
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == readableBytes());
        }
    }

private:
    std::vector<char> buffer_;      // vector用于替代固定大小数组
    size_t readerIndex_;            // 读位置
    size_t writerIndex_;            // 写位置

    static const char kCRLF[];      // "\r\n"

};

}       // namespace net 

}       // namespace muduo

#endif  // MUDUO_NET_BUFFER_H