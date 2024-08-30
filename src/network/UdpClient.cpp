#include "UdpClient.h"
#include "network/base/SocketOpt.h"
#include "network/base/Network.h"

using namespace lss::network;

// 初始化基类 UdpSocket，并保存服务器地址 server
UdpClient::UdpClient(EventLoop *loop, const InetAddress &server)
    : UdpSocket(loop, -1, InetAddress(), server)        // 调用基类构造函数，初始化 UDP 套接字
    , server_addr_(server)                              // 初始化服务器地址
{

}

// Connect 方法，在事件循环中异步调用 ConnectInLoop 方法进行连接
void UdpClient::Connect()
{
    loop_->RunInLoop([this](){
        // 在事件循环中执行连接操作
        ConnectInLoop();
    });
}

void UdpClient::SetConnectedCallback(const ConnectedCallback &cb)
{
    // 将传入的回调函数赋值给成员变量 connected_cb_
    connected_cb_ = cb;
}

void UdpClient::SetConnectedCallback(ConnectedCallback &&cb)
{
    // 右值引用赋值，使用 std::move 转移所有权
    connected_cb_ = std::move(cb);
}

// ConnectInLoop 方法，实际执行连接的地方，必须在事件循环中调用
void UdpClient::ConnectInLoop()
{
    // 确保在事件循环线程中调用
    loop_->AssertInLoopThread();

    // 创建一个非阻塞 UDP 套接字，返回文件描述符
    fd_ = SocketOpt::CreateNonblockingUdpSocket(AF_INET);

    // 如果文件描述符小于 0，表示创建失败
    if (fd_ < 0)
    {
        // 调用 OnClose 进行错误处理
        OnClose();

        return;
    }

    // 连接标志位设为 true
    connected_ = true;

    // 将当前客户端对象添加到事件循环中，处理事件
    loop_->AddEvent(std::dynamic_pointer_cast<UdpClient>(shared_from_this()));

    // 创建 SocketOpt 对象以管理套接字选项
    SocketOpt opt(fd_);

    // 使用套接字选项类的 Connect 方法连接到服务器
    opt.Connect(server_addr_);

    // 获取服务器的地址信息，并存储在 sock_addr_ 中
    server_addr_.GetSockAddr((struct sockaddr *)&sock_addr_);

    // 如果设置了连接成功的回调函数，则执行该回调
    if (connected_cb_)
    {
        connected_cb_(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()), true);
    }
}

// Send 方法，接受一个 BufferNodePtr 列表
void UdpClient::Send(std::list<BufferNodePtr> &list)
{

}

// Send 方法，发送数据，接受一个字符数组和大小
void UdpClient::Send(const char *buff, size_t size)
{
    UdpSocket::Send(buff, size, (struct sockaddr *)&sock_addr_, sock_len_);
}

void UdpClient::OnClose()
{
    // 如果连接已建立
    if (connected_)
    {
        // 从事件循环中删除当前客户端对象，停止事件处理
        loop_->DelEvent(std::dynamic_pointer_cast<UdpClient>(shared_from_this()));

        // 将连接标志位设为 false
        connected_ = false;
        
        // 调用基类的 OnClose 方法进行关闭处理
        UdpSocket::OnClose();
    }
}

UdpClient::~UdpClient()
{

}