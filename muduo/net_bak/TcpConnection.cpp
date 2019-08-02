#include <muduo/net/TcpConnection.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Socket.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
    << conn->peerAddress().toIpPort() << " is "
    << (conn->connected() ? "UP" : "DOWN");
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
    : loop_(CHECK_NOTNULL(loop)),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024)
{
    // 通道可读事件到来的时候，回调TcpConnection::handleRead，_1是事件发生时间
    channel_->setReadCallback(
        boost::bind(&TcpConnection::handleRead, this, _1));
    // 通道可写事件到来的时候，回调TcpConnection::handleWrite
    channel_->setWriteCallback(
        boost::bind(&TcpConnection::handleWrite, this));
    // 连接关闭，回调TcpConnection::handleClose
    channel_->setCloseCallback(
        boost::bind(&TcpConnection::handleClose, this));
    // 发生错误，回调TcpConnection::handleError
    channel_->setErrorCallback(
        boost::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this
        << " fd=" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this
              << " fd=" << channel_->fd();
}

// 线程安全，可以跨线程调用
void TcpConnection::send(const void* data, size_t len)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(data, len);
        }
        else
        {
            string message(static_cast<const char*>(data), len);    // 跨线程调用由一定的开销
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this,
                            message));
        }
    }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(const StringPiece& message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this,
                            message.as_string()));
                            //std::forward<string>(message)));
        }
    }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(Buffer* buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();     // 缓冲区数据移出
        }
        else
        {
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this,
                            buf->retrieveAllAsString()));
                            //std::forward<string>(message)));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    /*
    loop_->assertInLoopThread();
    sockets::write(channel_->fd(), data, len);
    */
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool error = false;
    if (state_ == kDisconnected)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    // if no thing in output queue, try writing directly
    // 通道没有关注可写事件并且发送缓冲区没有数据，直接write
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)   // 全部发送完数据
            {
                loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));    // 回调发送完回调函数
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if (errno == EPIPE) // FIXME: any others?
                {
                    error = true;
                }
            }
        }
    }

    assert(remaining <= len);
    // 没有错误，并且还有未写完的数据（说明内核发送缓冲区满，要将未写完的数据添加到output buffer中）
    if (!error && remaining > 0)
    {
        LOG_TRACE << "I am going to write more data";
        size_t oldLen = outputBuffer_.readableBytes();
        // 如果超过highWaterMark_（高水位标），回调highWaterMarkCallback_
        if (oldLen + remaining >= highWaterMark_
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(boost::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        if (!channel_->isWriting())         // 还有数据要发送, 检查是否关注POLLOUT
        {
            channel_->enableWriting();      // 若没有关注POLLOUT, 则关注POLLOUT事件
        }
    }
}

void TcpConnection::shutdown()
{
    // FIXME: use compare and swap
    if (state_ == kConnected)       // 判断当前状态为连接态
    {
        setState(kDisconnecting);   // 修改为正在断开状态
        // FIXME: shared_from_this()?
        loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())     // 不再关注POLLOUT事件就可以关闭写, 否则不能关闭, TcpConnection::handleWrite会把数据写完后再调用此函数
    {
        // we are not writing
        socket_->shutdownWrite();
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::connectEstablished()
{
    /* TcpConnection对象本身无法用use_cout, 需转换为shared_ptr */
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);      // 断言连接状态 
    setState(kConnected);
    LOG_TRACE << "[3] usecount=" << shared_from_this().use_count(); // 当前对象转换为shared_ptr对象, 引用计数+1, 引用计数=3, 由于是临时对象, 创建后立即销毁, 之后变为2
    channel_->tie(shared_from_this());  // 当前TcpConnection对象转换成shared_ptr对象, tie中弱引用不会更改计数, 引用计数+1=3, 临时对象销毁变为2 
    channel_->enableReading();          // TcpConnection所对应的通道加入到Poller关注

    connectionCallback_(shared_from_this());    // 连接建立时用户回调函数, 临时对象建立又销毁, 引用计数不变 
    LOG_TRACE << "[4] usecount=" << shared_from_this().use_count(); // 当前对象转换为shared_ptr对象, 引用计数+1, 引用计数=3, 由于是临时对象, 创建后立即销毁, 之后变为2
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());    // 实际上不会回调用户回调函数, 因为handleClose已经把状态设置为kDisconnected, if条件进不去 
    }
    channel_->remove();                             // Channel从Poller中移除
}

void TcpConnection::handleRead(Timestamp receiveTime)
{

    // 使用Buffer缓冲区 
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }

    /* 
    // 定义缓冲区
    loop_->assertInLoopThread();
    int savedErrno = 0;
    char buf[65536];
    ssize_t n = ::read(channel_->fd(), buf, sizeof buf);    // 电平触发, 先读取, close一样先读取
    if (n > 0)
    {
        messageCallback_(shared_from_this(), buf, n);       // shared_from_this(), 把当前tcp对象转换为shared_ptr
    }
    else if (n == 0)	// 连接断开
    {
        handleClose();	// 处理连接断开
    }
    else				// 处理错误
    {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
    */
}

// 内核发送缓冲区有空间了，回调该函数, POLLOUT事件触发了
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting())
    {
        ssize_t n = sockets::write(channel_->fd(),      // 不确定是否能把所有数据写入
            outputBuffer_.peek(),
            outputBuffer_.readableBytes());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)     // 发送缓冲区已清空
            {
                channel_->disableWriting();     // 停止关注POLLOUT事件，以免出现busy loop
                if (writeCompleteCallback_)     // 回调writeCompleteCallback_
                {
                    // 应用层发送缓冲区被清空，就回调用writeCompleteCallback_
                    loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)   // 发送缓冲区已清空并且连接状态是kDisconnecting(之前调用过TcpConnection::shutdown), 要关闭连接
                {
                    shutdownInLoop();           // 前面发生完已经disableWriting了, shutdownInLoop会关闭连接
                }
            }
            else
            {
                LOG_TRACE << "I am going to write more data";
            }
        }
        else
        {
            LOG_SYSERR << "TcpConnection::handleWrite";
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
        }
    }
    else
    {
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
    assert(state_ == kConnected || state_ == kDisconnecting);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);                        // connectionCallback_若注释, 这行状态改变也应该注释掉
    channel_->disableAll();

    //TcpConnectionPtr guardThis(This);             // 相当于又构造一个独立的shared_ptr对象, 引用计数=1, 而不是+1
    TcpConnectionPtr guardThis(shared_from_this()); // 返回自身对象的shared_ptr, shared_ptr被接收后引用计数=3
    connectionCallback_(guardThis);                 // 这一行，可以不调用, 之后在connectDestroyed回调用户到来回调函数
    LOG_TRACE << "[7] usecount=" << guardThis.use_count();
    // must be the last line
    closeCallback_(guardThis);                      // 内部连接断开回调函数, 调用TcpServer::removeConnection
    LOG_TRACE << "[11] usecount=" << guardThis.use_count(); // 引用计数=3
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}