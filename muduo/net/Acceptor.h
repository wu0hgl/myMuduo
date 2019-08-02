#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <muduo/net/Channel.h>
#include <muduo/net/Socket.h>

namespace muduo
{

namespace net
{

class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : boost::noncopyable
{
public:
    typedef boost::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    {
        newConnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_; } // acceptChannel_.enableReading 关注可读事件
    void listen();

private:
    void handleRead();

    EventLoop   *loop_;             // acceptChannel_所属的EventLoop
    Socket      acceptSocket_;      // 监听套接字   
    Channel     acceptChannel_;     // 通道, 观察套接字的可读事件
    NewConnectionCallback newConnectionCallback_;
    bool        listenning_;
    int         idleFd_;            // 预先准备一个空闲的文件描述符
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_ACCEPTOR_H