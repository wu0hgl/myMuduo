#ifndef MUDUO_NET_HTTP_HTTPSERVER_H
#define MUDUO_NET_HTTP_HTTPSERVER_H

#include <muduo/net/TcpServer.h>
#include <boost/noncopyable.hpp>

namespace muduo
{

namespace net
{

class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class HttpServer : boost::noncopyable       // 对外提供消息到来时回调函数绑定
{
public:
    typedef boost::function<void(const HttpRequest&,
                                 HttpResponse*)> HttpCallback;

    HttpServer(EventLoop* loop,
               const InetAddress& listenAddr,
               const string& name);

    ~HttpServer();  // force out-line dtor, for scoped_ptr members.

    /// Not thread safe, callback be registered before calling start().
    void setHttpCallback(const HttpCallback& cb)
    { httpCallback_ = cb; }

    void setThreadNum(int numThreads)     // 支持多线程
    { server_.setThreadNum(numThreads); }

    void start();

private:
    void onConnection(const TcpConnectionPtr& conn);    // 一个连接和一个上下文绑定到一起
    void onMessage(const TcpConnectionPtr& conn,
                   Buffer* buf,
                   Timestamp receiveTime);
    void onRequest(const TcpConnectionPtr&, const HttpRequest&);

    TcpServer server_;          // 应用层使用http协议, 传输控制层使用tcp协议
    HttpCallback httpCallback_;	// 在处理http请求（即调用onRequest）的过程中回调此函数，对请求进行具体的处理
    // 服务端收到客户端发送的http请求首先回调onMessage回调onRequest回调用户httpCallback_
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPSERVER_H