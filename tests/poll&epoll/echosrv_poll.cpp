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
    signal(SIGCHLD, SIG_IGN);   // ���⽩������

    int idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
    int listenfd;

    //if ((listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    if ((listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK/*������*/ | SOCK_CLOEXEC/*�����滻ʱ�ļ��������ر�*/, IPPROTO_TCP)) < 0)
        ERR_EXIT("socket");

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5188);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* ���õ�ַ�ظ����� */
    int on = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        ERR_EXIT("setsockopt");

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        ERR_EXIT("bind");
    if (listen(listenfd, SOMAXCONN) < 0)
        ERR_EXIT("listen");

    struct pollfd pfd;
    pfd.fd = listenfd;
    pfd.events = POLLIN;    // ��עPOLLIN�¼�

    PollFdList pollfds;     // ʹ��vector����Ϊɾ��ʱ���Զ����ƶ��Ĳ�����֤�ռ���������, ��ͨ������ƿռ��鷳
    pollfds.push_back(pfd);

    int nready;

    struct sockaddr_in peeraddr;
    socklen_t peerlen;
    int connfd;

    while (1)
    {
        nready = poll(&*pollfds.begin(), pollfds.size(), -1);   // C++ 11��׼stlȡ��ַpollfds.data()
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
            // acceptû�к���ĵ���������
            connfd = accept4(listenfd, (struct sockaddr*)&peeraddr, &peerlen, SOCK_NONBLOCK | SOCK_CLOEXEC);    // �����ļ�������������, ����SOCK_CLOEXEC���

            /*          if (connfd == -1)
            ERR_EXIT("accept4");
            */

            if (connfd == -1)
            {
                if (errno == EMFILE)                        // �ļ�������������
                {
                    close(idlefd);                          // �ճ�һ���ļ�������
                    idlefd = accept(listenfd, NULL, NULL);  // ����accept�����ļ�������
                    close(idlefd);
                    idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);   // �ָ�һ�������ļ�������
                    continue;
                }
                else                                        // ����״̬����Ҫ�˳�����, �˴��򵥴���
                    ERR_EXIT("accept4");
            }

            pfd.fd = connfd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            pollfds.push_back(pfd);
            --nready;

            // ���ӳɹ�
            std::cout << "ip=" << inet_ntoa(peeraddr.sin_addr) <<
                " port=" << ntohs(peeraddr.sin_port) << std::endl;
            if (nready == 0)
                continue;
        }

        //std::cout<<pollfds.size()<<std::endl;
        //std::cout<<nready<<std::endl;
        /* ����poll��ע���¼���100��, ��ֻ������3���¼�, Ҫ������100���ҳ���3���׽��ֲ����¼� */
        for (PollFdList::iterator it = pollfds.begin() + 1; it != pollfds.end() && nready > 0; ++it)    // �����������׽���
        {
            if (it->revents & POLLIN)       // ��������pollfd��reventsΪPOLLIN
            {
                --nready;                   // ����һ���¼�
                connfd = it->fd;
                char buf[1024] = { 0 };
                int ret = (int)read(connfd, buf, 1024);
                if (ret == -1)
                    ERR_EXIT("read");
                if (ret == 0)               // ��ȡΪ0, ˵���Է��ر��׽���
                {
                    std::cout << "client close" << std::endl;
                    it = pollfds.erase(it); // �׽��ֹر���, Ҫ��pollfd�Ƴ�, �´β��ٹ�ע��
                    // erase�������ָ����һλ��, ��for��++it, Ϊ�˲�����¼�Ҫ�ѵ�����-1
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
