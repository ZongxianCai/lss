#include "UdpServer.h"
#include "network/base/Network.h"

using namespace lss::network;

UdpServer::UdpServer(EventLoop *loop, const InetAddress &server)
    : UdpSocket(loop, -1, server, InetAddress())        // 调用基类 UdpSocket 的构造函数，初始化父类部分
    , server_(server)                                   // 初始化成员变量 server_
{

}

void UdpServer::Start()
{
    // 在事件循环中运行 Open 方法，延迟执行服务器的启动操作
    loop_->RunInLoop([this](){ // 使用 lambda 捕获 this 指针
        Open();                // 调用私有方法 Open 进行初始化
    });
}

void UdpServer::Stop()
{
    // 在事件循环中运行停止操作
    loop_->RunInLoop([this](){
        // 从事件循环中删除当前的 UdpSocket 对象
        loop_->DelEvent(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
        // 执行关闭操作
        OnClose();
    });
}

// Open 方法，打开服务器进行绑定和事件监听
void UdpServer::Open()
{
    // 确保在事件循环线程中执行此方法
    loop_->AssertInLoopThread();

    // 创建一个非阻塞 UDP 套接字，AF_INET 表示使用 IPv4
    fd_ = SocketOpt::CreateNonblockingUdpSocket(AF_INET);

    // 如果套接字创建失败
    if (fd_ < 0)
    {
        // 调用关闭操作
        OnClose();

        // 返回，不再继续执行后续操作
        return;
    }

    // 将当前的 UdpSocket 对象添加到事件循环中进行事件处理
    loop_->AddEvent(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));

    // 创建一个 SocketOpt 对象用于操作套接字选项
    SocketOpt opt(fd_);

    // 将套接字绑定到指定的服务器地址
    opt.BindAddress(server_);
}

UdpServer::~UdpServer()
{
    // 在析构时停止服务器
    Stop();
}