#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory>
#include "InetAddress.h"

namespace lss
{
    namespace network
    {
        // 定义 InetAddressPtr 为 InetAddress 的智能指针类型
        using InetAddressPtr = std::shared_ptr<InetAddress>;

        class SocketOpt
        {
        public:
            // 构造函数，接收 socket 描述符和 IPv6 标志
            SocketOpt(int sock, bool v6 = false);

            // 创建非阻塞 TCP socket 的静态方法
            static int CreateNonblockingTcpSocket(int family);

            // 创建非阻塞 UDP socket 的静态方法
            static int CreateNonblockingUdpSocket(int family);
            
            // 绑定地址的方法
            int BindAddress(const InetAddress &localaddr);

            // 开始监听的方法
            int Listen();

            // 接收连接的方法
            int Accept(InetAddress *peeraddr);

            // 连接到指定地址的方法
            int Connect(const InetAddress &addr);

            // 获取本地地址的方法
            InetAddressPtr GetLocalAddr();

            // 获取远端地址的方法
            InetAddressPtr GetPeerAddr();

            // 设置 TCP 无延迟选项的方法
            void SetTcpNoDelay(bool on);

            // 设置地址重用选项的方法
            void SetReuseAddr(bool on);

            // 获取 socket 描述符的方法
            void SetReusePort(bool on);

            // 关闭 socket 的方法
            void SetKeepAlive(bool on);

            // 获取 socket 的状态
            void SetNonBlocking(bool on);

            // 获取 socket 的类型
            ~SocketOpt() = default;

        private:
            // socket 描述符
            int sock_;

            // IPv6 标志
            bool is_v6_{false};
        };
    }
}