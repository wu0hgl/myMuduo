#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <sstream>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;   // 可读事件, 紧急事件
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd__)
    : loop_(loop),
    fd_(fd__),
    events_(0),
    revents_(0),
    index_(-1),                 // 初始状态-1, 还没有添加到poll/epoll中
    logHup_(true),
    tied_(false),
    eventHandling_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
}

void Channel::tie(const boost::shared_ptr<void>& obj)
{
    tie_ = obj;                     // tie_弱引用, 不会更改引用计数
    tied_ = true;
}

void Channel::update()
{
    loop_->updateChannel(this);
}

// 调用这个函数之前确保调用disableAll
void Channel::remove()
{
    assert(isNoneEvent());
    loop_->removeChannel(this);
}

// 事件到来时, 会调用
void Channel::handleEvent(Timestamp receiveTime)
{
    boost::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock();        // weak_ptr引用提升为shared_ptr, 引用计数+1=2
        if (guard)
        {
            LOG_TRACE << "[6] usecount=" << guard.use_count();  // 增加引用计数打印
            handleEventWithGuard(receiveTime);
            LOG_TRACE << "[12] usecount=" << guard.use_count(); // 引用计数=2
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}   // guard局部对象销毁, 引用计数-1=1
//剩余一个引用计数保存在: boost::function<void()> Functor, 函数调用之后引用计数-1, TcpConnection对象销毁

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    eventHandling_ = true;
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))       // 读事件不会产生POLLHUP
    {
        if (logHup_)
        {
            LOG_WARN << "Channel::handle_event() POLLHUP";
        }
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & POLLNVAL)                                // 文件描述符没有打开, 或者非法文件描述符, 没有退出程序
    {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))          // POLLRDHUP对等端关闭连接, 不论整个连接还是半连接, readCallback_会返回0
    {
        if (readCallback_) readCallback_(receiveTime);
    }
    if (revents_ & POLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
}

string Channel::reventsToString() const
{
    std::ostringstream oss;
    oss << fd_ << ": ";
    if (revents_ & POLLIN)
        oss << "IN ";
    if (revents_ & POLLPRI)
        oss << "PRI ";
    if (revents_ & POLLOUT)
        oss << "OUT ";
    if (revents_ & POLLHUP)
        oss << "HUP ";
    if (revents_ & POLLRDHUP)
        oss << "RDHUP ";
    if (revents_ & POLLERR)
        oss << "ERR ";
    if (revents_ & POLLNVAL)
        oss << "NVAL ";

    return oss.str().c_str();
}
