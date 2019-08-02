#include <muduo/net/TcpServer.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Acceptor.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>          // snprintf

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg)
    : loop_(CHECK_NOTNULL(loop)),           // Logging.h确保loop不为NULL
    hostport_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr)),
    threadPool_(new EventLoopThreadPool(loop)),
    connectionCallback_(defaultConnectionCallback),     // 默认回调函数, 在TcpConnection中定义
    messageCallback_(defaultMessageCallback),           // 默认回调函数, 在TcpConnection中定义
    started_(false),
    nextConnId_(1)
{
    // Acceptor::handleRead函数中会回调用TcpServer::newConnection
    // _1对应的是socket文件描述符，_2对应的是对等方的地址(InetAddress)
    acceptor_->setNewConnectionCallback(boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (ConnectionMap::iterator it(connections_.begin());
        it != connections_.end(); ++it)
    {
        TcpConnectionPtr conn = it->second;
        it->second.reset();     // 释放当前所控制的对象，引用计数减一
        conn->getLoop()->runInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
        conn.reset();           // 释放当前所控制的对象，引用计数减一
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);      // 设置线程池中IO线程的个数, 不包含mainReactor所属的IO线程, 实际线程数量numThreads+1
}

// 该函数多次调用是无害的
// 该函数可以跨线程调用
void TcpServer::start()
{
    if (!started_)      // 判断started_, 所以可以多次调用
    {
        started_ = true;
        threadPool_->start(threadInitCallback_);    // 线程初始化回调函数, 通过setThreadInitCallback来设置
    }

    if (!acceptor_->listenning())   // 第二次调用start时, acceptor_->listening()处于true状态
    {
        // get_pointer返回原生指针
        loop_->runInLoop(           // runInLoop, 可以跨线程调用
            boost::bind(&Acceptor::listen, get_pointer(acceptor_)));    // get_pointer(acceptor_)返回原生指针, 通过原生指针调用listen函数
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    loop_->assertInLoopThread();
    // 按照轮叫的方式选择一个EventLoop
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_
        << "] - new connection [" << connName
        << "] from " << peerAddr.toIpPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    //TcpConnectionPtr conn(new TcpConnection(loop_,      // 创建TcpConnection对象, 使用shared_ptr接管
    //                                        connName,
    //                                        sockfd,
    //                                        localAddr,
    //                                        peerAddr));
    TcpConnectionPtr conn(new TcpConnection(ioLoop,     // 所属的loop就是IO loop, 而不是loop_(acceptor所属loop)
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    LOG_TRACE << "[1] usecount=" << conn.use_count();       // 引用计数=1
    connections_[connName] = conn;                          // 加入到列表中
    LOG_TRACE << "[2] usecount=" << conn.use_count();       // 引用计数=2
    conn->setConnectionCallback(connectionCallback_);       // 连接到来回调函数
    conn->setMessageCallback(messageCallback_);             // 消息到来回调函数
    conn->setWriteCompleteCallback(writeCompleteCallback_); // 数据发送完毕回调函数

    conn->setCloseCallback(
        boost::bind(&TcpServer::removeConnection, this, _1));
    
    //conn->connectEstablished();       // 直接调用意味着在当前IO线程内调用, 应该让ioLoop所属的IO线程调用connectEstablished
    ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
    LOG_TRACE << "[5] usecount=" << conn.use_count();
}   // conn是一个临时对象, 跳出newConnection后, 引用计数为1, 列表connections_列表中有一个shared_ptr对象

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    /*
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
    << "] - connection " << conn->name();


    LOG_TRACE << "[8] usecount=" << conn.use_count();
    size_t n = connections_.erase(conn->name());
    LOG_TRACE << "[9] usecount=" << conn.use_count();

    (void)n;
    assert(n == 1);

    loop_->queueInLoop(
    boost::bind(&TcpConnection::connectDestroyed, conn));
    LOG_TRACE << "[10] usecount=" << conn.use_count();
    */

    loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));

}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
        << "] - connection " << conn->name();


    LOG_TRACE << "[8] usecount=" << conn.use_count();           // 引用计数不变, 仍为3
    size_t n = connections_.erase(conn->name());                // 从列表中移除, 引用计数-1 
    LOG_TRACE << "[9] usecount=" << conn.use_count();           // 引用计数-1=2

    (void)n;
    assert(n == 1);

    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        boost::bind(&TcpConnection::connectDestroyed, conn));

    //loop_->queueInLoop(
    //    boost::bind(&TcpConnection::connectDestroyed, conn));   // boost::bind得到一个boost::function对象, 引用计数+1
    LOG_TRACE << "[10] usecount=" << conn.use_count();          // 引用计数=3
}