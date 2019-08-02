﻿#ifndef MUDUO_NET_HTTP_HTTPRESPONSE_H
#define MUDUO_NET_HTTP_HTTPRESPONSE_H

#include <muduo/base/copyable.h>
#include <muduo/base/Types.h>

#include <map>

namespace muduo
{

namespace net
{

class Buffer;
        
class HttpResponse : public muduo::copyable
{
public:
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,               // 成功
        k301MovedPermanently = 301, // 301重定向，请求的页面永久性移至另一个地址
        k400BadRequest = 400,       // 错误的请求，语法格式有错，服务器无法处理此请求
        k404NotFound = 404,         // 请求的网页不存在
    };

    explicit HttpResponse(bool close)
        : statusCode_(kUnknown),
        closeConnection_(close)
    {
    }

    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    void setStatusMessage(const string& message)
    { statusMessage_ = message; }

    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    bool closeConnection() const
    { return closeConnection_; }

    // 设置文档媒体类型（MIME）
    void setContentType(const string& contentType)
    { addHeader("Content-Type", contentType); }

    // FIXME: replace string with StringPiece
    void addHeader(const string& key, const string& value)
    { headers_[key] = value; }

    void setBody(const string& body)
    { body_ = body; }

    void appendToBuffer(Buffer* output) const;	// 将HttpResponse添加到Buffer

private:
    std::map<string, string> headers_;  // header列表
    HttpStatusCode statusCode_;         // 状态响应码
    // FIXME: add http version
    string statusMessage_;              // 状态响应码对应的文本信息
    bool closeConnection_;              // 是否关闭连接, 短连接需要关闭连接
    string body_;                       // 实体
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_HTTP_HTTPRESPONSE_H