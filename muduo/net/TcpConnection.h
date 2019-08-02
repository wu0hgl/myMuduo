﻿#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include <muduo/base/Mutex.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>

#include <boost/any.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

/*
    ConnectionCallback  connectionCallback_;
        void TcpConnection::connectEstablished()
        void TcpConnection::connectDestroyed()
        void TcpConnection::handleClose()

    MessageCallback     messageCallback_;
        void TcpConnection::handleRead(Timestamp receiveTime)

    CloseCallback       closeCallback_; 回调TcpServer::removeConnection函数, 而不是回调应用层关闭函数
        void TcpConnection::handleClose()

    应用层通过bool connected() const { return state_ == kConnected; }  判断state_状态来进行回调
*/

namespace muduo
{

namespace net
{

class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : boost::noncopyable,
                      public boost::enable_shared_from_this<TcpConnection>    // 当前对象this指针转换为shared_ptr
{
public:
    /// Constructs a TcpConnection with a connected sockfd
    ///
    /// User should not create this object.
    TcpConnection(EventLoop* loop,
                 const string& name,
                 int sockfd,
                 const InetAddress& localAddr,
                 const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const string& name() const { return name_; }
    const InetAddress& localAddress() { return localAddr_; }
    const InetAddress& peerAddress() { return peerAddr_; }
    bool connected() const { return state_ == kConnected; }     

    // void send(string&& message); // C++11
    void send(const void* message, size_t len);
    void send(const StringPiece& message);
    // void send(Buffer&& message); // C++11
    void send(Buffer* message);     // this one will swap data
    void shutdown();                // NOT thread safe, no simultaneous calling, 服务端主动断开与客户端的连接, 这意味着客户端read返回0, 会close(conn)
    void setTcpNoDelay(bool on);

    void setContext(const boost::any& context)
    { context_ = context; }

    const boost::any& getContext() const
    { return context_; }

    boost::any* getMutableContext()
    { return &context_; }

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    Buffer* inputBuffer()
    { return &inputBuffer_; }

    /// Internal use only.
    void setCloseCallback(const CloseCallback& cb)	// 内部使用
    { closeCallback_ = cb; }

    // called when TcpServer accepts a new connection
    void connectEstablished();  // should be called only once
    // called when TcpServer has removed me from its map
    void connectDestroyed();    // should be called only once, 会把connectDestroy放到functors列表当中

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();         // Channel中可能会有些错误事件
    void sendInLoop(const StringPiece& message);
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();
    void setState(StateE s) { state_ = s; }

    EventLoop   *loop_;         // 所属EventLoop
    string      name_;          // 连接名
    StateE      state_;         // FIXME: use atomic variable
    // we don't expose those classes to client.
    boost::scoped_ptr<Socket>   socket_;
    boost::scoped_ptr<Channel>  channel_;
    InetAddress     localAddr_;
    InetAddress     peerAddr_;
    ConnectionCallback  connectionCallback_;
    MessageCallback     messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;   // 数据发送完毕回调函数，即所有的用户数据都已拷贝到内核缓冲区时回调该函数
                                                    // outputBuffer_被清空也会回调该函数，可以理解为低水位标回调函数
    HighWaterMarkCallback highWaterMarkCallback_;   // 高水位标回调函数, writeCompleteCallback_调用不及时, outputBuffer_不断增大, 
                                                    // 使用这个函数可以断开与对方连接, 避免内存不断增大
    CloseCallback       closeCallback_;		// Callback.h中定义, 内部连接断开回调函数, 即TcpServer中removeConnection回调函数, 不是用户外部回调函数
    size_t highWaterMark_;      // 高水位标
    Buffer inputBuffer_;        // 应用层接收缓冲区
    Buffer outputBuffer_;       // 应用层发送缓冲区
    boost::any context_;        // 绑定一个未知类型的上下文对象
};

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;  // 会不会与Callbacks.h中TcpConnectionPtr重复?

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_TCPCONNECTION_H