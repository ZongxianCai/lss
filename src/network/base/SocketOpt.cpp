#include "SocketOpt.h"
#include "Network.h"

using namespace lss::network;

SocketOpt::SocketOpt(int sock, bool v6)
: sock_(sock)
, is_v6_(v6)
{

}

int SocketOpt::CreateNonblockingTcpSocket(int family)
{
    int sock = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);

    if (sock < 0)
    {
        NETWORK_ERROR << " socket failed.";
    }

    return sock;
}

int SocketOpt::CreateNonblockingUdpSocket(int family)
{
    int sock = ::socket(family, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);

    if (sock < 0)
    {
        NETWORK_ERROR << " socket failed."; 
    }

    return sock;
}

int SocketOpt::BindAddress(const InetAddress &localaddr)
{
    if (localaddr.IsIpV6())
    {
        struct sockaddr_in6 addr;
        localaddr.GetSockAddr((struct sockaddr*)&addr);
        socklen_t size = sizeof(struct sockaddr_in6);

        return ::bind(sock_, (struct sockaddr*)&addr, size);
    }
    else
    {
        struct sockaddr_in addr;
        localaddr.GetSockAddr((struct sockaddr*)&addr);
        socklen_t size = sizeof(struct sockaddr_in);

        return ::bind(sock_, (struct sockaddr*)&addr, size);
    }
    
}

int SocketOpt::Listen()
{
    return ::listen(sock_, SOMAXCONN);
}

int SocketOpt::Accept(InetAddress *peeraddr)
{
    struct sockaddr_in6 addr;
    socklen_t len = sizeof(struct sockaddr_in6);
    int sock = ::accept4(sock_, (struct sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

    if (sock > 0)
    {
        if (addr.sin6_family == AF_INET)
        {
            char ip[16] = {0,};
            struct sockaddr_in *saddr = (struct sockaddr_in*)&addr;
            ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
            peeraddr->SetAddr(ip);
            peeraddr->SetPort(ntohs(saddr->sin_port));
        }
        else if (addr.sin6_family == AF_INET6)
        {
            char ip[INET6_ADDRSTRLEN] = {0,};
            ::inet_ntop(AF_INET6, &(addr.sin6_addr), ip, sizeof(ip));
            peeraddr->SetAddr(ip);
            peeraddr->SetPort(ntohs(addr.sin6_port));
            peeraddr->SetIsIPV6(true);
        } 
    }

    return sock;
}

int SocketOpt::Connect(const InetAddress &addr)
{
    struct sockaddr_in6 addr_in;
    addr.GetSockAddr((struct sockaddr*)&addr_in);
    return ::connect(sock_, (struct sockaddr*)&addr_in, sizeof(struct sockaddr_in6));
}

InetAddressPtr SocketOpt::GetLocalAddr()
{
    struct sockaddr_in6 addr_in;
    socklen_t len = sizeof(struct sockaddr_in6);
    ::getsockname(sock_, (struct sockaddr*)&addr_in, &len);
    InetAddressPtr peeraddr = std::make_shared<InetAddress>();
    
    if (addr_in.sin6_family == AF_INET)
    {
        char ip[16] = {0,};
        struct sockaddr_in *saddr = (struct sockaddr_in*)&addr_in;
        ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
        peeraddr->SetAddr(ip);
        peeraddr->SetPort(ntohs(saddr->sin_port));
    }
    else if (addr_in.sin6_family == AF_INET6)
    {
        char ip[INET6_ADDRSTRLEN] = {0, };
        ::inet_ntop(AF_INET6, &(addr_in.sin6_addr), ip, sizeof(ip));
        peeraddr->SetAddr(ip);
        peeraddr->SetPort(ntohs(addr_in.sin6_port));
        peeraddr->SetIsIPV6(true);
    }

    return peeraddr;
}

InetAddressPtr SocketOpt::GetPeerAddr()
{
    struct sockaddr_in6 addr_in;
    socklen_t len = sizeof(struct sockaddr_in6);
    ::getpeername(sock_, (struct sockaddr*)&addr_in, &len);
    InetAddressPtr peeraddr = std::make_shared<InetAddress>();
    
    if (addr_in.sin6_family == AF_INET)
    {
        char ip[16] = {0,};
        struct sockaddr_in *saddr = (struct sockaddr_in*)&addr_in;
        ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
        peeraddr->SetAddr(ip);
        peeraddr->SetPort(ntohs(saddr->sin_port));
    }
    else if (addr_in.sin6_family == AF_INET6)
    {
        char ip[INET6_ADDRSTRLEN] = {0, };
        ::inet_ntop(AF_INET6, &(addr_in.sin6_addr), ip, sizeof(ip));
        peeraddr->SetAddr(ip);
        peeraddr->SetPort(ntohs(addr_in.sin6_port));
        peeraddr->SetIsIPV6(true);
    }

    return peeraddr;
}

void SocketOpt::SetTcpNoDelay(bool on)
{
    int opt_value = on ? 1 : 0;
    ::setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, &opt_value, sizeof(opt_value));
}

void SocketOpt::SetReuseAddr(bool on)
{
    int opt_value = on ? 1 : 0;
    ::setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &opt_value, sizeof(opt_value));
}

void SocketOpt::SetReusePort(bool on)
{
    int opt_value = on ? 1 : 0;
    ::setsockopt(sock_, SOL_SOCKET, SO_REUSEPORT, &opt_value, sizeof(opt_value));
}

void SocketOpt::SetKeepAlive(bool on)
{
    int opt_value = on ? 1 : 0;
    ::setsockopt(sock_, SOL_SOCKET, SO_KEEPALIVE, &opt_value, sizeof(opt_value));
}

void SocketOpt::SetNonBlocking(bool on)
{
    int flag = ::fcntl(sock_, F_GETFL, 0);
    if (on)
    {
        flag |= O_NONBLOCK;
    }
    else
    {
        flag &= ~O_NONBLOCK;
    }
    ::fcntl(sock_, F_SETFL, flag);
}