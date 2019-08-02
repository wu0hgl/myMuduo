#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include <muduo/base/Types.h>
#include <muduo/net/TcpConnection.h>

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

/*
    TcpServer并不直接与Channel打交道, 而是通过Acceptor类来和TcpConnection与Channel进行交互
*/

namespace muduo
{

namespace net
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.
class TcpServer : boost::noncopyable
{
public:
    typedef boost::function<void(EventLoop*)> ThreadInitCallback;

    //TcpServer(EventLoop* loop, const InetAddress& listenAddr);
    TcpServer(EventLoop* loop,
              const InetAddress& listenAddr,
              const string& nameArg);
    ~TcpServer();  // force out-line dtor, for scoped_ptr members.

    const string& hostport() const { return hostport_; }    // 返回服务端口
    const string& name() const { return name_; }            // 返回服务名称 

    /// Set the number of threads for handling input.
    ///
    /// Always accepts new connection in loop's thread.
    /// Must be called before @c start
    /// @param numThreads
    /// - 0 means all I/O in loop's thread, no thread will created.
    ///   this is the default value.
    /// - 1 means all I/O in another thread.
    /// - N means a thread pool with N threads, new connections
    ///   are assigned on a round-robin basis.
    void setThreadNum(int numThreads);
    void setThreadInitCallback(const ThreadInitCallback& cb)
    { threadInitCallback_ = cb; }

    /// Starts the server if it's not listenning.
    ///
    /// It's harmless to call it multiple times.
    /// Thread safe.
    void start();                                           // 开启线程池以及acceptor_的listen套接字监听

    /// Set connection callback.
    /// Not thread safe.
    // 设置连接到来或者连接关闭回调函数
    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    /// Set message callback.
    /// Not thread safe.
    // 设置消息到来回调函数
    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    /// Set write complete callback.
    /// Not thread safe.
    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

private:
    /// Not thread safe, but in loop
    void newConnection(int sockfd, const InetAddress& peerAddr);    // 连接到来时的回调函数
    /// Thread safe.
    void removeConnection(const TcpConnectionPtr& conn);
    /// Not thread safe, but in loop
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    /* key: 连接名称, value: 连接对象的指针,  */
    typedef std::map<string, TcpConnectionPtr> ConnectionMap;       // typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

    EventLoop *loop_;           // the acceptor loop, acceptor_所属的EventLoop, 不一定是连接所属的EventLoop
    const string hostport_;     // 服务端口
    const string name_;         // 服务名
    boost::scoped_ptr<Acceptor> acceptor_;              // avoid revealing Acceptor
    boost::scoped_ptr<EventLoopThreadPool> threadPool_;
    ConnectionCallback      connectionCallback_;        // 连接到来回调函数
    MessageCallback         messageCallback_;           // 消息到来回调函数
    WriteCompleteCallback   writeCompleteCallback_;     // 数据发送完毕，会回调此函数
    ThreadInitCallback      threadInitCallback_;        // IO线程池中的线程在进入事件循环前，会回调用此函数
    bool started_;
    // always in loop thread
    int nextConnId_;            // 下一个连接ID
    ConnectionMap connections_;	// 连接列表
};


}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_TCPSERVER_H