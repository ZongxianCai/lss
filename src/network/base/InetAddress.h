#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <bits/socket.h>
#include <string>
#include <iostream>

namespace lss
{
    namespace network
    {
        class InetAddress
        {
        public:
            // 构造函数，根据 IP 地址、端口号和是否为 IPv6 地址创建 InetAddress 对象
            InetAddress(const std::string &ip, uint16_t port, bool bv6 = false);

            // 构造函数，根据主机名和是否为 IPv6 地址创建 InetAddress 对象
            InetAddress(const std::string &host, bool is_v6 = false);

            // 默认构造函数
            InetAddress() = default;

            // 默认析构函数
            ~InetAddress() = default;

            // 设置主机名
            void SetHost(const std::string &host);

            // 设置地址
            void SetAddr(const std::string &addr);

            // 设置端口号
            void SetPort(uint16_t port);

            // 设置是否为 IPv6 地址
            void SetIsIPV6(bool is_v6);

            // 获取 IP 地址
            const std::string &IP() const;

            // 获取 IPv4 地址
            uint32_t IPv4() const;

            // 获取 IP 地址和端口号的字符串表示
            std::string ToIpPort() const;

            // 获取端口号
            uint16_t Port() const;

            // 获取套接字地址
            void GetSockAddr(struct sockaddr *saddr) const;

            // 判断是否为IPv6地址
            bool IsIpV6() const;

            // 判断是否为广域网IP地址
            bool IsWanIp() const;

            // 判断是否为局域网IP地址
            bool IsLanIp() const;

            // 判断是否为回环IP地址
            bool IsLoopbackIp() const;

            // 获取主机的IP地址和端口号
            static void GetIpAndPort(const std::string &host, std::string &ip, std::string &port);

        private:
            // 将IPv4地址转换为32位整数
            uint32_t IPv4(const char *ip) const;

            // 地址
            std::string addr_;

            // 端口号
            std::string port_;

            // 是否为IPv6地址
            bool is_ipv6_{false};
        };
    }
}