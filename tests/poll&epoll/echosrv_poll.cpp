#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <poll.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <vector>
#include <iostream>

#pragma GCC diagnostic ignored "-Wold-style-cast"

#define ERR_EXIT(m)         \
    do                      \
                {                       \
        perror(m);          \
        exit(EXIT_FAILURE); \
                } while(0)

typedef std::vector<struct pollfd> PollFdList;

int main(void)
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);   // 避免僵死进程

    int idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
    int listenfd;

    //if ((listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    if ((listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK/*非阻塞*/ | SOCK_CLOEXEC/*进程替换时文件描述符关闭*/, IPPROTO_TCP)) < 0)
        ERR_EXIT("socket");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5188);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* 设置地址重复利用 */
    int on = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        ERR_EXIT("setsockopt");

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind");
    if (listen(listenfd, SOMAXCONN) < 0)
        ERR_EXIT("listen");

    struct pollfd pfd;
    pfd.fd = listenfd;
    pfd.events = POLLIN;    // 关注POLLIN事件

    PollFdList pollfds;     // 使用vector是因为删除时会自动做移动的操作保证空间是连续的, 普通数组控制空间麻烦
    pollfds.push_back(pfd);

    int nready;

    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int connfd;

    while (1)
    {
        nready = poll(&*pollfds.begin(), pollfds.size(), -1);   // C++ 11标准stl取地址pollfds.data()
        if (nready == -1)
        {
            if (errno == EINTR)
                continue;

            ERR_EXIT("poll");
        }
        if (nready == 0)    // nothing happended
            continue;

        if (pollfds[0].revents & POLLIN)
        {
            peerlen = sizeof(peeraddr);
            // accept没有后面的第三个参数
            connfd = accept4(listenfd, (struct sockaddr*)&peeraddr, &peerlen, SOCK_NONBLOCK | SOCK_CLOEXEC);    // 返回文件描述符非阻塞, 具有SOCK_CLOEXEC标记

            /*          if (connfd == -1)
            ERR_EXIT("accept4");
            */

            if (connfd == -1)
            {
                if (errno == EMFILE)                        // 文件描述符用完了
                {
                    close(idlefd);                          // 空出一个文件描述符
                    idlefd = accept(listenfd, NULL, NULL);  // 调用accept接收文件描述符
                    close(idlefd);
                    idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);   // 恢复一个空闲文件描述符
                    continue;
                }
                else                                        // 其他状态不需要退出程序, 此处简单处理
                    ERR_EXIT("accept4");
            }

            pfd.fd = connfd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            pollfds.push_back(pfd);
            --nready;

            // 连接成功
            std::cout << "ip=" << inet_ntoa(peeraddr.sin_addr) <<
                " port=" << ntohs(peeraddr.sin_port) << std::endl;
            if (nready == 0)
                continue;
        }

        //std::cout<<pollfds.size()<<std::endl;
        //std::cout<<nready<<std::endl;
        /* 可能poll关注的事件有100个, 但只产生了3个事件, 要遍历这100个找出哪3个套接字产生事件 */
        for (PollFdList::iterator it = pollfds.begin() + 1; it != pollfds.end() && nready > 0; ++it)    // 遍历已连接套接字
        {
            if (it->revents & POLLIN)       // 遍历到的pollfd的revents为POLLIN
            {
                --nready;                   // 处理一个事件
                connfd = it->fd;
                char buf[1024] = { 0 };
                int ret = (int)read(connfd, buf, 1024);
                if (ret == -1)
                    ERR_EXIT("read");
                if (ret == 0)               // 读取为0, 说明对方关闭套接字
                {
                    std::cout << "client close" << std::endl;
                    it = pollfds.erase(it); // 套接字关闭了, 要把pollfd移除, 下次不再关注它
                    // erase后迭代器指向下一位置, 而for中++it, 为了不错过事件要把迭代器-1
                    --it;

                    close(connfd);
                    continue;
                }

                std::cout << buf;
                write(connfd, buf, strlen(buf));

            }
        }
    }

    return 0;
}
