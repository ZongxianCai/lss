#include "UdpSocket.h"
#include "network/base/Network.h"

using namespace lss::network;

// 初始化UdpSocket对象
UdpSocket::UdpSocket(EventLoop *loop, int socketfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : Connection(loop, socketfd, localAddr, peerAddr)   // 调用基类Connection的构造函数，loop: 事件循环对象，socketfd: 套接字文件描述符，localAddr: 本地地址，peerAddr: 远端地址
    , message_buffer_(message_buffer_size_)             // 初始化   message_buffer_
{

}

// 设置关闭连接时的回调函数
void UdpSocket::SetCloseCallback(const UdpSocketCloseConnectionCallback &cb)
{
    close_cb_ = cb;
}

// 设置关闭连接时的回调函数，使用右值引用来避免拷贝
void UdpSocket::SetCloseCallback(UdpSocketCloseConnectionCallback &&cb)
{
    close_cb_ = std::move(cb);
}

// 设置接收消息时的回调函数
void UdpSocket::SetRecvMsgCallback(const UdpSocketMessageCallback &cb)
{
    message_cb_ = cb;
}

// 设置接收消息时的回调函数，使用右值引用来避免拷贝
void UdpSocket::SetRecvMsgCallback(UdpSocketMessageCallback &&cb)
{
    message_cb_ = std::move(cb);
}

// 设置写操作完成时的回调函数
void UdpSocket::SetWriteCompleteCallback(const UdpSocketWriteCompleteCallback &cb)
{
    write_complete_cb_ = cb;
}

// 设置写操作完成时的回调函数，使用右值引用来避免拷贝
void UdpSocket::SetWriteCompleteCallback(UdpSocketWriteCompleteCallback &&cb)
{
    write_complete_cb_ = std::move(cb);
}

// 设置超时回调函数，并指定超时时间
void UdpSocket::SetTimeoutCallback(int timeout, const UdpSocketTimeoutCallback &cb)
{
    // 将当前对象转换为shared_ptr，用于在回调中使用
    auto us = std::dynamic_pointer_cast<UdpSocket>(shared_from_this());

    // 在指定的超时后执行回调函数
    loop_->RunAfter(timeout, [this, cb, us](){
        cb(us); // 调用回调函数
    });
}

// 右值引用
void UdpSocket::SetTimeoutCallback(int timeout, UdpSocketTimeoutCallback &&cb)
{
    auto us = std::dynamic_pointer_cast<UdpSocket>(shared_from_this());

    loop_->RunAfter(timeout, [this, cb, us](){
        cb(us);
    });
}

// 当超时发生时调用的函数，记录日志并关闭连接
void UdpSocket::OnTimeOut()
{
    // 记录日志
    NETWORK_TRACE << " host : " << peer_addr_.ToIpPort() << " timeout. close it.";
    // 关闭连接
    OnClose();
}

// 启用检查空闲超时的功能
void UdpSocket::EnableCheckIdleTimeout(int32_t max_time)
{
    // 创建一个超时条目，并将其与当前套接字关联
    auto tp = std::make_shared<UdpTimeoutEntry>(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
    // 设置最大空闲时间
    max_idle_time_ = max_time;
    // 保存超时条目
    timeout_entry_ = tp;
    // 将超时条目插入到循环中
    loop_->InsertEntry(max_time, tp);
}

// 延长套接字的生命周期
void UdpSocket::ExtendLife()
{
    // 获取超时条目
    auto tp = timeout_entry_.lock();

    // 如果超时条目存在
    if (tp)
    {
        // 更新其在循环中的位置
        loop_->InsertEntry(max_idle_time_, tp);
    }
}

// 发送数据，使用列表作为参数
void UdpSocket::Send(std::list<UdpBufferNodePtr> &list)
{
    loop_->RunInLoop([this, &list](){
        // 在循环中发送数据
        SendInLoop(list);
    });
}

// 发送数据，使用缓冲区、大小、地址和长度作为参数
void UdpSocket::Send(const char *buff, size_t size, struct sockaddr *addr, socklen_t len)
{
    loop_->RunInLoop([this, buff, size, addr, len](){
        // 在循环中发送数据
        SendInLoop(buff, size, addr, len);
    });
}

void UdpSocket::OnError(const std::string &msg)
{
    // 记录日志
    NETWORK_TRACE << " host : " << peer_addr_.ToIpPort() << " error : " << msg;
    // 关闭连接
    OnClose();
}

void UdpSocket::OnRead()
{
    // 如果套接字已经关闭
    if (closed_)
    {
        // 输出警告信息，指明该主机已经关闭
        NETWORK_WARNING << " host : " << peer_addr_.ToIpPort() << " had closed.";

        // 结束函数
        return;
    }

    // 循环处理接收的数据
    while (true)
    {
        // 定义用于存储接收地址信息的结构体
        struct sockaddr_in6 sock_addr;
        socklen_t len = sizeof(sock_addr);

        // 从套接字中接收数据
        auto ret = ::recvfrom(fd_, message_buffer_.BeginWrite(), message_buffer_size_, 0, (struct sockaddr *)&sock_addr, &len);

        // 如果成功接收到数据
        if (ret > 0)
        {
            // 创建一个 InetAddress 对象用于存储对端地址
            InetAddress peeraddr;
            // 更新缓冲区的已写入字节数
            message_buffer_.HasWritten(ret);

            // 根据地址族处理不同的地址格式
            if (sock_addr.sin6_family == AF_INET)
            {
                // IPv4 地址
                char ip[16] = {0, };
                // 将 sockaddr_in 转换为指针
                struct sockaddr_in *saddr = (struct sockaddr_in*)&sock_addr;
                // 将 IPv4 地址转换为字符串
                ::inet_ntop(AF_INET, &(saddr->sin_addr.s_addr), ip, sizeof(ip));
                // 设置对端地址
                peeraddr.SetAddr(ip);
                // 设置对端端口
                peeraddr.SetPort(ntohs(saddr->sin_port));
            }
            else if (sock_addr.sin6_family == AF_INET6)
            {
                // IPv6 地址
                char ip[INET6_ADDRSTRLEN] = {0, };
                // 将 IPv6 地址转换为字符串
                ::inet_ntop(AF_INET6, &(sock_addr.sin6_addr), ip, sizeof(ip));
                // 设置对端地址
                peeraddr.SetAddr(ip);
                // 设置对端端口
                peeraddr.SetPort(ntohs(sock_addr.sin6_port));
                // 标记为 IPv6 地址
                peeraddr.SetIsIPV6(true);
            }

            // 如果定义了消息回调函数
            if (message_cb_)
            {
                // 调用消息回调函数
                message_cb_(peeraddr, message_buffer_);
            }

            // 清空缓冲区
            message_buffer_.RetrieveAll();
        }
        else if (ret < 0)
        {
            // 如果接收数据出错
            if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                // 输出错误信息
                NETWORK_ERROR << " host : " << peer_addr_.ToIpPort() << " error : " << errno;
                // 处理关闭操作
                OnClose();

                // 结束函数
                return;
            }
            
            // 退出循环
            break;
        }
    }
}

void UdpSocket::OnWrite()
{
    // 如果套接字已经关闭
    if (closed_)
    {
        // 输出警告信息，指明该主机已经关闭
        NETWORK_WARNING << " host : " << peer_addr_.ToIpPort() << " had closed. ";

        // 结束函数
        return;
    }

    // 延长对象的生命周期，与事件循环相关
    ExtendLife();

    // 循环处理缓冲区中的数据
    while (true)
    {
        // 如果缓冲区不为空
        if (!buffer_list_.empty())
        {
            // 获取缓冲区中的第一个数据块
            auto buf = buffer_list_.front();
            // 发送数据
            auto ret = ::sendto(fd_, buf->addr, buf->size, 0, buf->sock_addr, buf->sock_len);

            // 如果数据发送成功
            if (ret > 0)
            {
                // 从缓冲区中移除已发送的数据块
                buffer_list_.pop_front();
            }
            // 如果发送失败
            else if (ret < 0)
            {
                // 如果错误不是由中断、资源暂时不可用等常见原因引起的
                if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    // 输出错误信息
                    NETWORK_ERROR << " host : " << peer_addr_.ToIpPort() << " error : " << errno;
                    // 处理关闭操作
                    OnClose();

                    // 结束函数
                    return;
                }

                // 如果是可恢复的错误，退出循环
                break;
            }
        }

        // 如果缓冲区为空
        if (buffer_list_.empty())
        {
            // 如果定义了写完成的回调函数
            if (write_complete_cb_)
            {
                // 调用写完成回调函数
                write_complete_cb_(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
            }

            // 退出循环
            break;
        }
    }
}

void UdpSocket::OnClose()
{
    // 如果套接字还未关闭
    if (!closed_)
    {
        // 标记套接字为关闭状态
        closed_ = true;

        // 如果定义了关闭回调函数
        if (close_cb_)
        {
            // 调用关闭回调函数
            close_cb_(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
        }

        // 调用事件的关闭处理函数
        Event::Close();
    }
}

void UdpSocket::ForceClose()
{
    // 在事件循环中执行关闭操作
    loop_->RunInLoop([this](){
        OnClose();
    });
}

void UdpSocket::SendInLoop(std::list<UdpBufferNodePtr> &list)
{
    // 将传入的缓冲区节点列表添加到套接字的缓冲区列表中
    for (auto &i : list)
    {
        buffer_list_.emplace_back(i);
    }

    // 如果缓冲区列表不为空，则启用写操作
    if (!buffer_list_.empty())
    {
        EnableWriting(true);
    }
}

void UdpSocket::SendInLoop(const char *buff, size_t size, struct sockaddr *saddr, socklen_t len)
{
    // 如果缓冲区列表为空，尝试立即发送数据
    if (buffer_list_.empty())
    {
        // 尝试发送数据
        auto ret = ::sendto(fd_, buff, size, 0, saddr, len);

        // 如果发送成功，直接返回
        if (ret > 0)
        {
            return ;
        }
    }

    // 如果立即发送失败，或者缓冲区列表不为空，将数据添加到缓冲区列表中
    auto node = std::make_shared<UdpBufferNode>((void*)buff, size, saddr, len);

    buffer_list_.emplace_back(node);

    // 如果立即发送失败，或者缓冲区列表不为空，将数据添加到缓冲区列表中
    EnableWriting(true);
}

UdpSocket::~UdpSocket()
{
    // 析构函数，释放 UdpSocket 对象时调用
}