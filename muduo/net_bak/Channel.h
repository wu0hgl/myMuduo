#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <muduo/base/Timestamp.h>

namespace muduo
{

namespace net
{

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : boost::noncopyable
{
public:
    typedef boost::function<void()> EventCallback;
    typedef boost::function<void(Timestamp)> ReadEventCallback; // 读事件处理多一个时间戳

    Channel(EventLoop* loop, int fd);				            // 一个EventLoop包含多个Channel, 但一个Channel只能由一个EventLoop负责
    ~Channel();

    /* 设置回调函数 */
    void handleEvent(Timestamp receiveTime);
    void setReadCallback(const ReadEventCallback& cb)
    {
        readCallback_ = cb;
    }
    void setWriteCallback(const EventCallback& cb)
    {
        writeCallback_ = cb;
    }
    void setCloseCallback(const EventCallback& cb)
    {
        closeCallback_ = cb;
    }
    void setErrorCallback(const EventCallback& cb)
    {
        errorCallback_ = cb;
    }

    /// Tie this channel to the owner object managed by shared_ptr,
    /// prevent the owner object being destroyed in handleEvent.
    void tie(const boost::shared_ptr<void>&);

    int fd() const { return fd_; }                      // Channel所对应的描述符
    int events() const { return events_; }              // 注册哪些事件保持在events_里面
    void set_revents(int revt) { revents_ = revt; }     // used by pollers
    // int revents() const { return revents_; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }

    /* 设置事件属性 */
    void enableReading() { events_ |= kReadEvent; update(); }
    // void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }       // 不关注事件
    bool isWriting() const { return events_ & kWriteEvent; }

    // for Poller
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // for debug
    string reventsToString() const;     // 事件转换成字符串以便调试

    void doNotLogHup() { logHup_ = false; }

    EventLoop* ownerLoop() { return loop_; }
    void remove();

private:
    void update();          // 把Channel注册到EventLoop中, 从而注册到Poller中
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;       // 所属EventLoop
    const int  fd_;         // 文件描述符，但不负责关闭该文件描述符
    int        events_;     // 关注的事件
    int        revents_;    // poll/epoll返回的事件
    int        index_;      // used by Poller.表示在poll的事件数组中的序号, 小于0表示新增的事件, 还没有添加到数组中; 在epoll中表示通道的状态
    bool       logHup_;     // for POLLHUP

    boost::weak_ptr<void> tie_;     //tie_类型为void, 任意类型指针
    bool tied_;
    bool eventHandling_;    // 是否处于处理事件中
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

}       // net

}       // namespace muduo

#endif  // MUDUO_NET_CHANNEL_H