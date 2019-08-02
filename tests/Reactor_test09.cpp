#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

/*
    Reactor_test08.cpp对象模式, 使用基于对象模型, 单线程模型
    文件描述符:
        0, 1, 2
        3 PollerFd
        4 timerFd
        5 wakeupFd
        6 listenFd
        7 idleFd
        8 已连接
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
        if (conn->connected())  // 连接建立
        {
            printf("onConnection(): new connection [%s] from %s\n",
                conn->name().c_str(),
                conn->peerAddress().toIpPort().c_str());
        }
        else                    // 连接断开
        {
            printf("onConnection(): connection [%s] is down\n",
                conn->name().c_str());
        }
    }

    void onMessage(const TcpConnectionPtr& conn,
        const char* data,
        ssize_t len)
    {
        printf("onMessage(): received %zd bytes from connection [%s]\n",
            len, conn->name().c_str());
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