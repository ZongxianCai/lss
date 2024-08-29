#include <functional>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <cstring>
#include "DnsService.h"

using namespace lss::network;

// 匿名命名空间，表示其中声明的标识符具有内部链接，仅在同一翻译单元（源文件）中可访问
namespace
{
    // static 关键字表示变量 inet_address_null 具有内部链接，不能从其他源文件访问，确保它在此文件中是唯一的
    static InetAddressPtr inet_address_null;
}

void DnsService::AddHost(const std::string& host)
{
    // 使用 std::lock_guard 来管理互斥量 lock_，确保在函数执行期间对共享资源的独占访问，自动处理锁的获取和释放
    std::lock_guard<std::mutex> lk(lock_);

    // 在 hosts_info_ 容器中查找 host，返回一个迭代器 iter
    auto iter = hosts_info_.find(host);

    // 检查是否存在
    if (iter != hosts_info_.end())
    {
        // 已存在，函数直接返回，不做任何操作
        return;
    }
    
    // 如果 host 不存在，则在 hosts_info_ 中添加 host，并初始化为一个空的 InetAddressPtr 向量
    hosts_info_[host] = std::vector<InetAddressPtr>();
}

InetAddressPtr DnsService::GetHostAddress(const std::string &host, int index)
{
    // 使用 std::lock_guard 管理互斥量 lock_，确保线程安全
    std::lock_guard<std::mutex> lk(lock_);

    // 在 hosts_info_ 中查找主机名
    auto iter = hosts_info_.find(host);

    // 检查主机名是否存在
    if (iter != hosts_info_.end())
    {
        // 获取与主机名关联的地址列表
        auto list = iter->second;

        // 如果列表不为空
        if (list.size() > 0)
        {
            // 返回指定索引的地址，使用取模运算符处理索引超出范围的情况
            return list[index % list.size()];
        }
    }

    // 如果未找到主机名或列表为空，返回空地址
    return inet_address_null;
}

std::vector<InetAddressPtr> DnsService::GetHostAddress(const std::string &host)
{
    // 使用 std::lock_guard 管理互斥量 lock_，确保线程安全
    std::lock_guard<std::mutex> lk(lock_);

    // 在 hosts_info_ 中查找主机名
    auto iter = hosts_info_.find(host);

    // 检查主机名是否存在
    if (iter != hosts_info_.end())
    {
        // 获取与主机名关联的地址列表
        auto list = iter->second;

        // 返回地址列表
        return list;
    }

    // 如果未找到主机名，返回一个空的地址列表
    return std::vector<InetAddressPtr>();
}

void DnsService::UpdateHost(const std::string &host, std::vector<InetAddressPtr> &list)
{
    // 使用 std::lock_guard 管理互斥量 lock_，确保线程安全
    std::lock_guard<std::mutex> lk(lock_);

    // 通过交换操作，将传入的地址列表替换掉与主机名关联的现有地址列表
    hosts_info_[host].swap(list);
}

std::unordered_map<std::string, std::vector<InetAddressPtr>> DnsService::GetHosts()
{
    // 使用 std::lock_guard 管理互斥量 lock_，确保线程安全
    std::lock_guard<std::mutex> lk(lock_);

    // 返回当前所有主机及其关联的 IP 地址列表
    return hosts_info_;
}

void DnsService::SetDnsServiceParam(int32_t interval, int32_t sleep, int32_t retry)
{
    // 设置 DNS 服务的时间间隔
    interval_ = interval;
    // 设置 DNS 服务的休眠时间
    sleep_ = sleep;
    // 设置 DNS 服务的重试次数
    retry_ = retry;
}

void DnsService::Start()
{
    // 设置服务状态为运行中
    running_ = true;
    // 创建一个新线程，执行 OnWork 方法
    thread_ = std::thread(std::bind(&DnsService::OnWork, this));
}

void DnsService::Stop()
{
    // 设置服务状态为停止
    running_ = false;

    // 如果线程可连接，则等待线程完成
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void DnsService::OnWork()
{
    // 当服务运行时持续执行
    while (running_)
    {
        // 获取主机信息
        auto host_infos = GetHosts();
        
        // 遍历每个主机
        for (auto &host : host_infos)
        {
            std::vector<InetAddressPtr> list;
            // 获取主机详细信息
            GetHostInfo(host.first, list);
            // 如果获取到信息，则更新主机
            if (list.size() > 0)
            {
                UpdateHost(host.first, list);

                // 更新后跳出循环
                break;
            }

            // 如果没有信息，休眠一段时间再尝试
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_));
        }
    }
    // 循环结束后休眠一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(interval_));
}

void DnsService::GetHostInfo(const std::string &host, std::vector<InetAddressPtr> &list)
{
    // 定义 addrinfo 结构体并初始化
    struct addrinfo ainfo, *res;
    // 清空结构体
    memset(&ainfo, 0x00, sizeof(ainfo));
    // 支持 IPv4 和 IPv6
    ainfo.ai_family = AF_UNSPEC;
    // 被动模式
    ainfo.ai_flags = AI_PASSIVE;
    // 数据报套接字
    ainfo.ai_socktype = SOCK_DGRAM;

    // 获取主机信息
    auto ret = ::getaddrinfo(host.c_str(), nullptr, &ainfo, &res);

    // 检查返回值
    if (ret == -1 || res == nullptr)
    {
        // 出错则返回
        return;
    }

    // 指向结果链表的指针
    struct addrinfo *rp = res;

    // 遍历 addrinfo 链表
    for (; rp != nullptr; rp = rp->ai_next)
    {
        // 创建 InetAddress 对象
        InetAddressPtr peeraddr = std::make_shared<InetAddress>();

        // 如果是 IPv4
        if (rp->ai_family == AF_INET)
        {
            // 定义 IPv4 地址缓冲区
            char ip[16] = {0, };
            // 转换为 sockaddr_in
            struct sockaddr_in *saddr = (struct sockaddr_in *)rp->ai_addr;
            // 转换为字符串格式
            ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
            // 设置地址
            peeraddr->SetAddr(ip);
            // 设置端口
            peeraddr->SetPort(ntohs(saddr->sin_port));
        }
        // 如果是 IPv6
        else if (rp->ai_family == AF_INET6)
        {
            // 定义 IPv6 地址缓冲区
            char ip[INET6_ADDRSTRLEN] = {0, };
            // 转换为 sockaddr_in6
            struct sockaddr_in6 *saddr = (struct sockaddr_in6 *)rp->ai_addr;
            // 转换为字符串格式
            ::inet_ntop(AF_INET6, &(saddr->sin6_addr), ip, sizeof(ip));
            // 设置地址
            peeraddr->SetAddr(ip);
            // 设置端口
            peeraddr->SetPort(ntohs(saddr->sin6_port));
            // 标记为 IPv6
            peeraddr->SetIsIPV6(true);
        }
        // 将地址添加到列表中
        list.push_back(peeraddr);
    }
}

DnsService::~DnsService()
{

}