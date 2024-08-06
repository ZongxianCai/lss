#include "SocketOpt.h"
#include "Network.h"

using namespace lss::network;

SocketOpt::SocketOpt(int sock, bool v6)
: sock_(sock)
, is_v6_(v6)
{

}

// 创建非阻塞 TCP 套接字的函数
int SocketOpt::CreateNonblockingTcpSocket(int family)
{
    // 创建一个非阻塞的TCP套接字
    int sock = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);

    // 如果套接字创建失败
    if (sock < 0)
    {
        // 记录错误信息
        NETWORK_ERROR << " socket failed.";
    }

    // 返回创建的套接字
    return sock;
}

// 创建非阻塞 UDP 套接字的函数
int SocketOpt::CreateNonblockingUdpSocket(int family)
{
    // 创建一个非阻塞的 UDP 套接字
    int sock = ::socket(family, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);

    // 如果套接字创建失败
    if (sock < 0)
    {
        // 记录错误信息
        NETWORK_ERROR << " socket failed."; 
    }

    // 返回创建的套接字
    return sock;
}

// 定义 BindAddress 函数，接收一个 InetAddress 类型的引用参数 localaddr
int SocketOpt::BindAddress(const InetAddress &localaddr)
{
    // 检查 localaddr 是否为 IPv6 地址
    if (localaddr.IsIpV6())
    {
        // 定义一个 sockaddr_in6 结构体用于 IPv6 地址
        struct sockaddr_in6 addr;
        // 从 localaddr 获取 sockaddr 结构体并填充 addr
        localaddr.GetSockAddr((struct sockaddr*)&addr);
        // 获取 sockaddr_in6 结构体的大小
        socklen_t size = sizeof(struct sockaddr_in6);
        
        // 调用bind函数将sock_绑定到addr
        return ::bind(sock_, (struct sockaddr*)&addr, size);
    }
    else
    {
        // 定义一个 sockaddr_in 结构体用于 IPv4 地址
        struct sockaddr_in addr;
        // 从 localaddr 获取 sockaddr 结构体并填充 addr
        localaddr.GetSockAddr((struct sockaddr*)&addr);
        // 获取 sockaddr_in 结构体的大小
        socklen_t size = sizeof(struct sockaddr_in);

        // 调用 bind 函数将 sock_ 绑定到 addr
        return ::bind(sock_, (struct sockaddr*)&addr, size);
    }
    
}

int SocketOpt::Listen()
{
    // 调用全局的 listen 函数，将 sock_ 作为参数传入，并使用 SOMAXCONN 作为最大连接数的参数
    // 该函数用于将套接字标记为被动套接字，以接收传入的连接请求
    return ::listen(sock_, SOMAXCONN);
}

// 接收一个传入的网络连接，并将连接的对端地址信息存储到 InetAddress 对象中
int SocketOpt::Accept(InetAddress *peeraddr)
{ 
    // 定义一个 sockaddr_in6 结构体变量 addr，用于存储对端的地址信息
    struct sockaddr_in6 addr;
    // 定义一个变量 len，用于存储地址结构的大小
    socklen_t len = sizeof(struct sockaddr_in6);
    // 调用 accept4 函数接受一个连接
    // sock_ 是当前 socket 的文件描述符，addr 用于存储对端地址，len 是地址长度
    // SOCK_NONBLOCK | SOCK_CLOEXEC 是标志位，表示以非阻塞模式接受连接并在执行时关闭子进程的文件描述符
    int sock = ::accept4(sock_, (struct sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

    // 成功接收到连接
    if (sock > 0)
    {
        // 检查地址族是否为 IPv4
        if (addr.sin6_family == AF_INET)
        {
            // 定义一个字符数组 ip 用于存储 IPv4 地址
            char ip[16] = {0,};
            // 将 addr 转换为 sockaddr_in 类型
            struct sockaddr_in *saddr = (struct sockaddr_in*)&addr;
            // 将二进制格式的 IPv4 地址转换为字符串格式
            ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
            // 将转换后的 IP 地址设置到 peeraddr 对象中
            peeraddr->SetAddr(ip);
            // 将端口号转换为网络字节序并设置到 peeraddr 对象中
            peeraddr->SetPort(ntohs(saddr->sin_port));
        }
        else if (addr.sin6_family == AF_INET6) // 检查地址族是否为 IPv6
        {
            // 定义一个字符数组 ip 用于存储 IPv6 地址
            char ip[INET6_ADDRSTRLEN] = {0,};
            // 将二进制格式的 IPv6 地址转换为字符串格式
            ::inet_ntop(AF_INET6, &(addr.sin6_addr), ip, sizeof(ip));
            // 将转换后的 IP 地址设置到 peeraddr 对象中
            peeraddr->SetAddr(ip);
            // 将端口号转换为网络字节序并设置到 peeraddr 对象中
            peeraddr->SetPort(ntohs(addr.sin6_port));
            // 标记 peeraddr 对象为 IPv6 地址
            peeraddr->SetIsIPV6(true);
        } 
    }

    // 返回接收到的 socket 文件描述符
    return sock;
}

// 连接到指定地址
int SocketOpt::Connect(const InetAddress &addr)
{
    // 定义一个 IPv6 地址结构体，用于存储 IPv6 地址信息
    struct sockaddr_in6 addr_in;
    // 从 InetAddress 获取套接字地址，并填充到 addr_in 中
    addr.GetSockAddr((struct sockaddr*)&addr_in);

    // 连接到指定地址，并返回连接结果
    return ::connect(sock_, (struct sockaddr*)&addr_in, sizeof(struct sockaddr_in6));
}

// 获取本地地址
InetAddressPtr SocketOpt::GetLocalAddr()
{
    // 定义一个 sockaddr_in6 结构体，用于存储本地地址信息
    struct sockaddr_in6 addr_in;
    // 定义长度变量，存储 sockaddr_in6 结构体的大小
    socklen_t len = sizeof(struct sockaddr_in6);
    // 获取本地套接字的地址并填充到 addr_in 中
    ::getsockname(sock_, (struct sockaddr*)&addr_in, &len);
    // 创建一个 InetAddress 的智能指针
    InetAddressPtr peeraddr = std::make_shared<InetAddress>();
    
    // 检查地址族是否为 IPv4
    if (addr_in.sin6_family == AF_INET)
    {
        // 定义一个字符数组，用于存储 IPv4 地址
        char ip[16] = {0,};
        // 将 addr_in 转换为 sockaddr_in 结构体指针
        struct sockaddr_in *saddr = (struct sockaddr_in*)&addr_in;
        // 将 IPv4 地址转换为字符串格式
        ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
        // 设置 peeraddr 的地址
        peeraddr->SetAddr(ip);
        // 设置 peeraddr 的端口
        peeraddr->SetPort(ntohs(saddr->sin_port));
    }
    else if (addr_in.sin6_family == AF_INET6) // 检查地址族是否为 IPv6
    {
        // 定义一个字符数组，用于存储 IPv6 地址
        char ip[INET6_ADDRSTRLEN] = {0, };
        // 将 IPv6 地址转换为字符串格式
        ::inet_ntop(AF_INET6, &(addr_in.sin6_addr), ip, sizeof(ip));
        // 设置远端地址
        peeraddr->SetAddr(ip);
        // 设置远端端口
        peeraddr->SetPort(ntohs(addr_in.sin6_port));
        // 设置远端为 IPv6 地址
        peeraddr->SetIsIPV6(true);
    }

    // 返回远端地址信息
    return peeraddr;
}

InetAddressPtr SocketOpt::GetPeerAddr()
{
    // 定义一个IPv6地址结构
    struct sockaddr_in6 addr_in;
    // 获取地址结构的大小
    socklen_t len = sizeof(struct sockaddr_in6);
    // 获取远端的地址信息
    ::getpeername(sock_, (struct sockaddr*)&addr_in, &len);
    // 创建一个InetAddress的智能指针
    InetAddressPtr peeraddr = std::make_shared<InetAddress>();
    
    // 如果是IPv4
    if (addr_in.sin6_family == AF_INET)
    {
        // 定义一个字符数组用于存储IPv4地址
        char ip[16] = {0,};
        // 将地址结构转换为IPv4结构
        struct sockaddr_in *saddr = (struct sockaddr_in*)&addr_in;
        // 将IPv4地址转换为字符串
        ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
        // 设置远端地址
        peeraddr->SetAddr(ip);
        // 设置远端端口
        peeraddr->SetPort(ntohs(saddr->sin_port));
    }
    else if (addr_in.sin6_family == AF_INET6) // 如果是IPv6
    {
        // 定义一个字符数组用于存储IPv6地址
        char ip[INET6_ADDRSTRLEN] = {0, };
        // 将IPv6地址转换为字符串
        ::inet_ntop(AF_INET6, &(addr_in.sin6_addr), ip, sizeof(ip));
        // 设置远端地址
        peeraddr->SetAddr(ip);
        // 设置远端端口
        peeraddr->SetPort(ntohs(addr_in.sin6_port));
        // 标记为IPv6地址
        peeraddr->SetIsIPV6(true);
    }

    // 返回远端地址信息
    return peeraddr;
}

void SocketOpt::SetTcpNoDelay(bool on)
{
    // 根据参数设置选项值，开启为1，关闭为0
    int opt_value = on ? 1 : 0;
    // 设置 TCP_NODELAY 选项
    ::setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, &opt_value, sizeof(opt_value));
}

void SocketOpt::SetReuseAddr(bool on)
{
    // 根据参数设置选项值，开启为1，关闭为0
    int opt_value = on ? 1 : 0;
    // 设置 SO_REUSEADDR 选项
    ::setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &opt_value, sizeof(opt_value));
}

void SocketOpt::SetReusePort(bool on)
{
    // 根据参数设置选项值，开启为1，关闭为0
    int opt_value = on ? 1 : 0;
    // 设置 SO_REUSEPORT 选项
    ::setsockopt(sock_, SOL_SOCKET, SO_REUSEPORT, &opt_value, sizeof(opt_value));
}

void SocketOpt::SetKeepAlive(bool on)
{
    // 根据参数设置选项值，开启为1，关闭为0
    int opt_value = on ? 1 : 0;
    // 设置 SO_KEEPALIVE 选项
    ::setsockopt(sock_, SOL_SOCKET, SO_KEEPALIVE, &opt_value, sizeof(opt_value));
}

void SocketOpt::SetNonBlocking(bool on)
{
    // 获取当前文件描述符的标志
    int flag = ::fcntl(sock_, F_GETFL, 0);

    // 如果要设置为非阻塞
    if (on)
    {
        // 添加非阻塞标志
        flag |= O_NONBLOCK;
    }
    else // 如果要设置为阻塞
    {
        // 移除非阻塞标志
        flag &= ~O_NONBLOCK;
    }

    // 设置新的标志
    ::fcntl(sock_, F_SETFL, flag);
}