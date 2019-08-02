#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include <muduo/base/copyable.h>
#include <muduo/base/StringPiece.h>

#include <netinet/in.h>

namespace muduo
{

namespace net
{

///
/// Wrapper of sockaddr_in.
///
/// This is an POD interface class.
class InetAddress : public muduo::copyable
{
public:
    /// Constructs an endpoint with given port number.
    /// Mostly used in TcpServer listening.
    // 仅仅指定port，不指定ip，则ip为INADDR_ANY（即0.0.0.0）, 一般用于监听
    explicit InetAddress(uint16_t port);

    /// Constructs an endpoint with given ip and port.
    /// @c ip should be "1.2.3.4", 即指定地址又指定端口
    InetAddress(const StringPiece& ip, uint16_t port);  // StringPiece 即可知道char*参数, 又可指定string参数

    /// Constructs an endpoint with given struct @c sockaddr_in
    /// Mostly used when accepting new connections
    InetAddress(const struct sockaddr_in& addr)
        : addr_(addr)
    { }

    string toIp() const;
    string toIpPort() const;

    // __attribute__ ((deprecated)) 表示该函数是过时的, 被淘汰的
    // 这样使用该函数, 在编译的时候, 会发出警告, muduo编译时警告转换为错误
    string toHostPort() const __attribute__((deprecated))
    {
        return toIpPort();
    }

    // default copy/assignment are Okay

    const struct sockaddr_in& getSockAddrInet() const { return addr_; }
    void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }

    uint32_t ipNetEndian() const { return addr_.sin_addr.s_addr; }  // 返回网络字节序32位整数
    uint16_t portNetEndian() const { return addr_.sin_port; }       // 返回网络字节序的端口

private:
    struct sockaddr_in addr_;
};

}       // namespace net

}       // namespace muduo

#endif  // MUDUO_NET_INETADDRESS_H