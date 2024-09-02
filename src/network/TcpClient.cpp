#include "TcpClient.h"
#include "network/base/Network.h"
#include "network/base/SocketOpt.h"

using namespace lss::network;

TcpClient::TcpClient(EventLoop *loop, const InetAddress &server)
    : TcpConnection(loop, -1, InetAddress(), server)        // 调用基类TcpConnection的构造函数
    , server_addr_(server)                                  // 初始化服务器地址                            
{

}

void TcpClient::Connect()
{
    // 在事件循环中运行连接操作
    loop_->RunInLoop([this](){
        // 调用ConnectInLoop函数
        ConnectInLoop();
    });
}

void TcpClient::SetConnectCallback(const ConnectionCallback &cb)
{
    // 将传入的回调函数赋值给connected_cb_
    connected_cb_ = cb;
}

void TcpClient::SetConnectCallback(ConnectionCallback &&cb)
{
    // 移动传入的回调函数
    connected_cb_ = std::move(cb);
}

void TcpClient::ConnectInLoop()
{
    // 确保在正确的线程中执行
    loop_->AssertInLoopThread();
    // 创建非阻塞TCP套接字
    fd_ = SocketOpt::CreateNonblockingTcpSocket(AF_INET);

    // 检查套接字是否创建成功
    if (fd_ < 0)
    {
        // 创建失败，关闭连接
        OnClose();

        // 退出函数
        return ;
    }

    // 更新状态为连接中
    status_ = kTcpConStatusConnecting;
    // 将当前TcpClient添加到事件循环中
    loop_->AddEvent(std::dynamic_pointer_cast<TcpClient>(shared_from_this()));

    // 启用写入
    EnableWriting(true);
    // 设置空闲超时为3秒
    // EnableCheckIdleTimeout(3);

    // 创建SocketOpt对象以操作套接字
    SocketOpt opt(fd_);
    // 尝试连接服务器
    auto ret = opt.Connect(server_addr_);

    // 如果连接成功
    if (ret == 0)
    {
        // 更新连接状态
        UpdateConnectionStatus();

        // 退出函数
        return;
    }
    // 如果连接失败
    else if (ret == -1)
    {
        // 如果错误不是正在进行中
        if (errno != EINPROGRESS)
        {
            // 记录错误信息
            NETWORK_ERROR << " connect to server : " << server_addr_.ToIpPort() << " error : " << errno;
            // 关闭连接
            OnClose();

            // 退出函数
            return ;
        }
    }
}

void TcpClient::UpdateConnectionStatus()
{
    // 更新状态为已连接
    status_ = kTcpConStatusConnected;

    // 如果有连接回调函数
    if (connected_cb_)
    {
        // 调用连接回调
        connected_cb_(std::dynamic_pointer_cast<TcpClient>(shared_from_this()), true);
    }
}
        
bool TcpClient::CheckError()
{
    // 初始化错误变量
    int error = 0;
    // 获取错误变量的大小
    socklen_t len = sizeof(error);
    // 获取套接字错误
    ::getsockopt(fd_, SOL_SOCKET, SO_ERROR, &error, &len);

    // 返回是否有错误
    return error != 0;
}

void TcpClient::OnRead()
{
    // 如果状态为连接中
    if (status_ == kTcpConStatusConnecting)
    {
        // 检查连接错误
        if (CheckError())
        {
            // 记录错误信息
            NETWORK_ERROR << " connect to server : " << server_addr_.ToIpPort() << " error : " << errno;
            // 关闭连接
            OnClose();

            // 退出函数
            return;
        }

        // 更新连接状态
        UpdateConnectionStatus();

        // 退出函数
        return ;
    }
    // 如果状态为已连接
    else if (status_ == kTcpConStatusConnected)
    {
        // 调用基类的读取处理
        TcpConnection::OnRead();
    }
}

void TcpClient::OnWrite()
{
    // 如果状态为连接中
    if (status_ == kTcpConStatusConnecting)
    {
        // 检查连接错误
        if (CheckError())
        {
            // 记录错误信息
            NETWORK_ERROR << " connect to server : " << server_addr_.ToIpPort() << " error : " << errno;
            // 关闭连接
            OnClose();

            // 退出函数
            return;
        }

        // 更新连接状态
        UpdateConnectionStatus();

        // 退出函数
        return ;
    }
    // 如果状态为已连接
    else if (status_ == kTcpConStatusConnected)
    {
        // 调用基类的写入处理
        TcpConnection::OnWrite();
    }
}

void TcpClient::OnClose()
{
    // 如果状态为连接中或已连接
    if (status_ == kTcpConStatusConnecting || status_ == kTcpConStatusConnected)
    {
        // 从事件循环中删除
        loop_->DelEvent(std::dynamic_pointer_cast<TcpClient>(shared_from_this()));
    }

    // 更新状态为已断开
    status_ = kTcpConStatusDisConnected;
    // 调用基类的关闭处理
    TcpConnection::OnClose();
}

void TcpClient::Send(std::list<BufferNodePtr> &list)
{
    // 如果状态为已连接
    if (status_ == kTcpConStatusConnected)
    {
        // 调用基类的发送处理
        TcpConnection::Send(list);
    }
}

void TcpClient::Send(const char *buff, size_t size)
{
    // 如果状态为已连接
    if(status_ == kTcpConStatusConnected)
    {
        // 调用基类的发送处理
        TcpConnection::Send(buff, size);
    }
}

TcpClient::~TcpClient()
{
    // 调用关闭函数
    OnClose();
}