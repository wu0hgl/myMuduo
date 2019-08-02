#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include <muduo/net/InetAddress.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{

namespace net
{

class Channel;
class EventLoop;

// 主动发起连接, 带有自动重连功能(连接还没有建立成功是否重连)
class Connector : boost::noncopyable,
                  public boost::enable_shared_from_this<Connector>
{
public:
    typedef boost::function<void(int sockfd)> NewConnectionCallback;

    Connector(EventLoop* loop, const InetAddress& serverAddr);
    ~Connector();

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

    void start();       // can be called in any thread
    void restart();     // must be called in loop thread
    void stop();        // can be called in any thread

    const InetAddress& serverAddress() const { return serverAddr_; }

private:
    enum States { kDisconnected, kConnecting, kConnected };
    static const int kMaxRetryDelayMs = 30 * 1000;      // 30秒，最大重连延迟时间
    static const int kInitRetryDelayMs = 500;           // 0.5秒，初始状态，连接不上，0.5秒后重连

    void setState(States s) { state_ = s; }
    void startInLoop();
    void stopInLoop();
    void connect();
    void connecting(int sockfd);
    void handleWrite();
    void handleError();
    void retry(int sockfd);     // 连接建立没有成功是否重连
    int removeAndResetChannel();
    void resetChannel();

    EventLoop   *loop_;         // 所属EventLoop
    InetAddress serverAddr_;    // 服务器端地址
    bool        connect_;       // atomic
    States      state_;         // FIXME: use atomic variable
    boost::scoped_ptr<Channel>  channel_;               // Connector所对应的Channel
    NewConnectionCallback       newConnectionCallback_; // 连接成功回调函数，
    int         retryDelayMs_;  // 重连延迟时间（单位：毫秒）
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_CONNECTOR_H