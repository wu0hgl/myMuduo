#include <muduo/net/Acceptor.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
    : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()),
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))  // 预先准备一个套接字
{
    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);           // 设置地址重复利用
    acceptSocket_.bindAddress(listenAddr);      // 绑定
    acceptChannel_.setReadCallback(             // 设置读回调函数
        boost::bind(&Acceptor::handleRead, this));  // 有些疑问, void setReadCallback(const ReadEventCallback& cb)回调函数类型应该是Timestamp
}                                                   // typedef boost::function<void(Timestamp)> ReadEventCallback;

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();    // 断言在IO线程中
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading(); // 关注可读事件
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);        // 对等端地址 
    //FIXME loop until no more
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        // string hostport = peerAddr.toIpPort();
        // LOG_TRACE << "Accepts of " << hostport;
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr);   // 回调上层用户回调函数
        }
        else
        {
            sockets::close(connfd);                     // 没有设置回调函数把文件描述符关闭
        }
    }
    else
    {
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of livev.
        if (errno == EMFILE)    // 文件描述符使用完, 使用电平触发, 若不处理会一直触发
        {
            ::close(idleFd_);                                       // 把空闲文件描述符关闭, 腾出文件描述符
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);     // 这时可接收
            ::close(idleFd_);                                       // 关闭
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);    // 继续等于空闲文件描述符
        }
    }
}