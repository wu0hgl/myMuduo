#ifndef MUDUO_NET_HTTP_HTTPCONTEXT_H
#define MUDUO_NET_HTTP_HTTPCONTEXT_H

#include <muduo/base/copyable.h>

#include <muduo/net/http/HttpRequest.h>

namespace muduo
{

namespace net
{

class HttpContext : public muduo::copyable
{
public:
    enum HttpRequestParseState
    {
        kExpectRequestLine, // 解析请求行状态
        kExpectHeaders,     // 解析头部状态
        kExpectBody,        // 解析实体状态, 当前没有处理带有实体的请求
        kGotAll,            // 解析完毕
    };

    HttpContext()
        : state_(kExpectRequestLine)
    {
    }

    // default copy-ctor, dtor and assignment are fine

    bool expectRequestLine() const      // 处于解析请求行状态
    { return state_ == kExpectRequestLine; }

    bool expectHeaders() const          // 处于解析头部状态
    { return state_ == kExpectHeaders; }

    bool expectBody() const             // 处于解析实体状态
    { return state_ == kExpectBody; }

    bool gotAll() const                 // 处于全部解析完毕状态
    { return state_ == kGotAll; }

    void receiveRequestLine()           // 设置kExpectHeaders状态
    { state_ = kExpectHeaders; }

    void receiveHeaders()               // 设置kGotAll状态
    { state_ = kGotAll; }  // FIXME

    void reset()                        // 通过HttpRequest::swap重置HttpContext状态
    {
        state_ = kExpectRequestLine;
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const  // 返回request_
    { return request_; }

    HttpRequest& request()
    { return request_; }

private:
    HttpRequestParseState state_;   // 请求解析状态
    HttpRequest request_;           // http请求, 对协议解析, 包含一个Request对象
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPCONTEXT_H