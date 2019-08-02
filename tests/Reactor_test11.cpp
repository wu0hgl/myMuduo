#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

/*
    回射服务器
*/

class TestServer
{
public:
    TestServer(EventLoop* loop,
        const InetAddress& listenAddr)
        : loop_(loop),
        server_(loop, listenAddr, "TestServer")
    {
        server_.setConnectionCallback(
            boost::bind(&TestServer::onConnection, this, _1));
        server_.setMessageCallback(
            boost::bind(&TestServer::onMessage, this, _1, _2, _3));
    }

    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        if (conn->connected())
        {
            printf("onConnection(): new connection [%s] from %s\n",
                conn->name().c_str(),
                conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            printf("onConnection(): connection [%s] is down\n",
                conn->name().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn,    // 与Reactor_test10相比, 接口发生变化
                   Buffer* buf,
                   Timestamp receiveTime)
    {
        string msg(buf->retrieveAllAsString());     // 获取缓冲区数据
        printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
            msg.size(),
            conn->name().c_str(),
            receiveTime.toFormattedString().c_str());
        conn->send(msg);                            // 回射数据
    }

    EventLoop* loop_;
    TcpServer server_;
};


int main()
{
    printf("main(): pid = %d\n", getpid());

    InetAddress listenAddr(8888);
    EventLoop loop;

    TestServer server(&loop, listenAddr);
    server.start();

    loop.loop();
}