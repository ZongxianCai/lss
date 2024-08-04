#include <cstring>
#include "InetAddress.h"
#include "Network.h"

using namespace lss::network;

// 构造函数，根据 IP 地址、端口号和是否为 IPv6 地址创建 InetAddress 对象
InetAddress::InetAddress(const std::string &ip, uint16_t port, bool bv6)
// 对象创建时直接对成员变量进行初始化，避免在构造函数体内使用赋值语句进行初始化的额外开销
: addr_(ip)                     // 设置地址
, port_(std::to_string(port))   // 设置端口号
, is_ipv6_(bv6)                 // 设置是否为IPv6地址
{

}

// 构造函数，根据主机名和是否为 IPv6 地址创建 InetAddress 对象
InetAddress::InetAddress(const std::string &host, bool is_v6)
{
    // 通过主机名获取 IP 地址和端口号
    GetIpAndPort(host, addr_, port_);
    // 设置是否为 IPv6 地址，默认为false
    is_ipv6_ = is_v6;
}

// 设置主机名
void InetAddress::SetHost(const std::string &host)
{
    // 调用 GetIpAndPort 函数获取 IP 地址和端口号
    GetIpAndPort(host, addr_, port_);
}

// 设置地址
void InetAddress::SetAddr(const std::string &addr)
{
    // 将传入的 addr 参数赋值给 addr_ 成员变量
    addr_ = addr;
}

// 设置端口号
void InetAddress::SetPort(uint16_t port)
{
    // 将端口号以字符串的形式存储，在需要使用端口号的地方可以直接使用字符串表示
    port_ = std::to_string(port);
}

// 设置是否为 IPv6 地址
void InetAddress::SetIsIPV6(bool is_v6)
{
    // 将传入的 is_v6 参数赋值给 is_v6_ 成员变量
    is_ipv6_ = is_v6;
}

// 获取 IP 地址
const std::string &InetAddress::IP() const
{
    return addr_;
}

// 获取 IPv4 地址
uint32_t InetAddress::IPv4(const char *ip) const
{
    // 存储IPv4地址和端口号的结构体
    struct sockaddr_in addr_in;
    // 将结构体的所有成员变量都初始化为 0
    memset(&addr_in, 0x00, sizeof(struct sockaddr_in));
    // 设置地址族为 IPv4
    addr_in.sin_family = AF_INET;
    // 设置端口号为 0 ，表示不使用端口号
    addr_in.sin_port = 0;

    // 如果将IP地址转换为网络字节序失败
    if (::inet_pton(AF_INET, ip, &addr_in.sin_addr) <= 0)
    {
        // 输出错误信息
        NETWORK_ERROR << " ipv4 ip : " << ip << " convert failed.";
    }

    // 将网络字节序的IP地址转换为主机字节序，并返回
    return ntohl(addr_in.sin_addr.s_addr);
}

// 获取 IPv4 地址
uint32_t InetAddress::IPv4() const
{
    return IPv4(addr_.c_str());
}

// 获取 IP 地址和端口号的字符串表示
std::string InetAddress::ToIpPort() const
{
    // 存储格式化后的字符串
    std::stringstream ss;
    // 使用 : 分割 IP 地址和端口号
    ss << addr_ << ":" << port_;
    
    // 返回 ss 对象的字符串表示形式
    return ss.str();
}

// 获取端口号
uint16_t InetAddress::Port() const
{
    // 将存储在 port_ 成员变量中的字符串转换为 int 类型并返回
    return std::atoi(port_.c_str());
}

// 获取套接字地址
void InetAddress::GetSockAddr(struct sockaddr *saddr) const
{
    // 如果是 IPv6 地址
    if (is_ipv6_)
    {
        // 将地址结构体的指针赋值给 addr_in6 变量
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)saddr;
        // 将结构体的所有成员变量都初始化为 0
        memset(&addr_in6, 0x00, sizeof(struct sockaddr_in6));
        // 将 sin6_family 字段设置为 AF_INET6 ，表示该地址结构体是用于 IPv6 地址
        addr_in6->sin6_family = AF_INET6;
        // 将 sin6_port 字段设置为通过将端口号从字符串转换为整数后，使用htons函数将其转换为网络字节序
        // 确保端口号在网络传输中以正确的字节顺序进行传输
        addr_in6->sin6_port = htons(std::atoi(port_.c_str()));
        if(::inet_pton(AF_INET6, addr_.c_str(), &addr_in6->sin6_addr) < 0)
        {
            // IPv6 地址转换失败
        }

        return;
    }

    // 如果是 IPv4 地址
    // 将地址结构体的指针赋值给 addr_in 变量
    struct sockaddr_in *addr_in = (struct sockaddr_in *)saddr;
    // 将结构体的所有成员变量都初始化为 0
    memset(addr_in, 0x00, sizeof(struct sockaddr_in));
    // 将sin_family字段设置为AF_INET ，表示该地址结构体是用于 IPv4 地址
    addr_in->sin_family = AF_INET;
    // 将 sin_port 字段设置为通过将端口号从字符串转换为整数后，使用htons函数将其转换为网络字节序
    // 确保端口号在网络传输中以正确的字节顺序进行传输
    addr_in->sin_port = htons(std::atoi(port_.c_str()));
    if(::inet_pton(AF_INET, addr_.c_str(), &addr_in->sin_addr) < 0)
    {
        // IPv4 地址转换失败
    }    
}

// 判断是否为IPv6地址
bool InetAddress::IsIpV6() const
{
    return is_ipv6_;
}

// 判断是否为广域网IP地址
bool InetAddress::IsWanIp() const
{
    uint32_t a_start = IPv4("10.0.0.0");
    uint32_t a_end = IPv4("10.255.255.255");

    uint32_t b_start = IPv4("172.16.0.0");
    uint32_t b_end = IPv4("172.31.255.255");

    uint32_t c_start = IPv4("192.168.0.0");
    uint32_t c_end = IPv4("192.168.255.255");

    uint32_t ip = IPv4();
    bool is_a = ip >= a_start && ip <= a_end;
    bool is_b = ip >= b_start && ip <= b_end;
    bool is_c = ip >= c_start && ip <= c_end;

    return !is_a && !is_b && !is_c && ip != INADDR_LOOPBACK;
}

// 判断是否为局域网IP地址
bool InetAddress::IsLanIp() const
{
    uint32_t a_start = IPv4("10.0.0.0");
    uint32_t a_end = IPv4("10.255.255.255");

    uint32_t b_start = IPv4("172.16.0.0");
    uint32_t b_end = IPv4("172.31.255.255");

    uint32_t c_start = IPv4("192.168.0.0");
    uint32_t c_end = IPv4("192.168.255.255");

    uint32_t ip = IPv4();
    bool is_a = ip >= a_start && ip <= a_end;
    bool is_b = ip >= b_start && ip <= b_end;
    bool is_c = ip >= c_start && ip <= c_end;

    return is_a || is_b || is_c;
}

// 判断是否为回环IP地址
bool InetAddress::IsLoopbackIp() const
{
    return addr_ == "127.0.0.1";
}

// 获取主机的 IP 地址和端口号
void InetAddress::GetIpAndPort(const std::string &host, std::string &ip, std::string &port)
{
    // 在host字符串中查找第一个冒号的位置
    auto pos = host.find_first_of(":", 0);

    // 如果找到了
    if (pos != std::string::npos)
    {
        // 提取冒号之前的部分作为 ip
        ip = host.substr(0, pos);
        // 提取冒号之后的部分作为 port
        port = host.substr(pos + 1);
    }
    else
    {
        // host 本身就是 ip
        ip = host;
    }
}