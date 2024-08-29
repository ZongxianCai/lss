#include <unistd.h>
#include "TcpConnection.h"
#include "network/base/Network.h"

using namespace lss::network;

TcpConnection::TcpConnection(EventLoop *loop, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
    : Connection(loop, sockfd, localAddr, peerAddr)     // 初始化基类 Connection，传递参数
{

}

void TcpConnection::SetCloseCallback(const CloseConnectionCallback &cb)
{
    // 将回调函数赋值给成员变量 close_cb_
    close_cb_ = cb;
}

void TcpConnection::SetCloseCallback(CloseConnectionCallback &&cb)
{
    // 使用 std::move 将右值引用的回调函数移动到成员变量 close_cb_
    close_cb_ = std::move(cb);
}

void TcpConnection::OnClose()
{
    // 确保当前线程是事件循环线程
    loop_->AssertInLoopThread();

    // 如果连接尚未关闭
    if (!closed_)
    {
        // 将连接状态标记为已关闭（放到此处，避免后续TcpClient超时导致重复进入）
        closed_ = true;

        // 如果存在关闭回调
        if (close_cb_)
        {
            // 调用关闭回调，并将当前对象转换为 TcpConnection 的共享指针
            close_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
        }
    }

    // 调用基类关闭函数，用于未执行到析构函数但需要在此处进行关闭的操作
    Event::Close();
}

void TcpConnection::ForceClose()
{
    // 在事件循环中调用 OnClose 函数
    /*
     * 线程安全：RunInLoop 确保 OnClose 在事件循环线程中执行，避免了跨线程调用可能导致的竞态条件和不一致状态。
     * 延迟执行：通过将 OnClose 封装在一个 lambda 表达式中，可以将其延迟到事件循环的下一个周期执行，允许当前操作完成后再处理关闭逻辑。
     * 捕获 this 指针：使用 [this] 捕获当前对象的指针，使得在 lambda 内部可以直接访问 TcpConnection 的成员函数和变量 
     */
    loop_->RunInLoop([this](){
        OnClose();
    });
}

void TcpConnection::OnRead()
{
    // 检查连接是否已关闭
    if (closed_)
    {
        // 记录日志，显示对端地址已关闭连接
        NETWORK_TRACE << " host : " << peer_addr_.ToIpPort() << " had closed.";
        // 直接返回，不再处理
        return;
    }

    // 开始一个无限循环，直到手动中断
    while (true)
    {
        // 初始化错误码
        int err = 0;
        // 从文件描述符 fd_ 中读取数据到 message_buffer_，并获取返回值和错误码
        auto ret = message_buffer_.ReadFd(fd_, &err);

        // 如果成功读取到数据
        if (ret > 0)
        {
            // 检查是否设置了消息回调
            if (message_cb_)
            {
                // 调用消息回调，传递当前对象的共享指针和消息缓冲区
                message_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()), message_buffer_);
            }
        }
        // 如果返回值为 0，表示对端关闭了连接
        else if (ret == 0)
        {
            // 调用关闭连接的函数
            OnClose();
            // 退出循环
            break;
        }
        else // 如果读取失败
        {
            // 检查错误码是否为中断、暂时不可用或非阻塞
            if (err != EINTR && err != EAGAIN && err != EWOULDBLOCK)
            {
                // 记录读取错误的日志
                // NETWORK_ERROR << " read err : " << err;
                // 调用关闭连接的函数
                OnClose(); 
            }

            // 退出循环
            break;
        }
    }
}

void TcpConnection::SetRecvMsgCallback(const MessageCallback &cb)
{
    // 将回调函数赋值给成员变量 message_cb_
    message_cb_ = cb;
}

void TcpConnection::SetRecvMsgCallback(MessageCallback &&cb)
{
    // 使用 std::move 将右值引用的回调函数移动到成员变量 message_cb_
    message_cb_ = std::move(cb);
}

void TcpConnection::OnError(const std::string &msg)
{
    // 记录错误日志，显示对端地址和错误消息
    NETWORK_ERROR << " host : " << peer_addr_.ToIpPort() << " error msg : " << msg;
    // 调用关闭连接的函数
    OnClose();
}

void TcpConnection::OnWrite()
{
    // 检查连接是否已关闭
    if (closed_)
    {
        // 记录日志，显示对端地址已关闭连接
        NETWORK_TRACE << " host : " << peer_addr_.ToIpPort() << " had closed.";
        // 直接返回，不再处理
        return;
    }

    // 检查待写入的数据列表是否为空
    if (!io_vec_list_.empty())
    {
        // 开始一个无限循环，直到手动中断
        while (true)
        {
            // 使用 writev 函数将数据写入文件描述符 fd_
            auto ret = ::writev(fd_, &io_vec_list_[0], io_vec_list_.size());

            // 如果写入成功
            if (ret >= 0)
            {
                // 处理写入的字节数
                while (ret > 0)
                {
                    // 如果当前数据块的长度大于已写入的字节数
                    if (io_vec_list_.front().iov_len > ret)
                    {
                        // 更新当前数据块的基地址，移动已写入的字节数
                        io_vec_list_.front().iov_base = (char*)io_vec_list_.front().iov_base + ret;
                        // 更新当前数据块的长度，减少已写入的字节数
                        io_vec_list_.front().iov_len -= ret;
                        // 退出内层循环
                        break;
                    }
                    else // 如果当前数据块的长度小于或等于已写入的字节数
                    {
                        // 减去当前数据块的长度
                        ret -= io_vec_list_.front().iov_len;
                        // 移除已写入的数据块
                        io_vec_list_.erase(io_vec_list_.begin());
                    }
                }

                // 如果所有数据块都已写入
                if (io_vec_list_.empty())
                {
                    // 禁用写入
                    EnableWriting(false);

                    // 检查是否设置了写入完成的回调
                    if (write_complete_cb_)
                    {
                        // 调用写入完成的回调，传递当前对象的共享指针
                        write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
                    }

                    // 退出函数
                    return;
                }
            }
            else // 如果写入失败
            {
                // 检查错误码是否为中断、暂时不可用或非阻塞
                if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    // 记录写入错误的日志
                    NETWORK_ERROR << " host : " << peer_addr_.ToIpPort() << " write err : " << errno;
                    // 调用关闭连接的函数
                    OnClose();
                    // 退出函数
                    return;
                }

                // 退出循环
                break;
            }
        }
    }
    else // 如果待写入的数据列表为空
    {
        // 禁用写入
        EnableWriting(false);

        // 检查是否设置了写入完成的回调
        if(write_complete_cb_)
        {
            // 调用写入完成的回调，传递当前对象的共享指针
            write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
        }
    }
}

void TcpConnection::SetWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    // 将回调函数赋值给成员变量 write_complete_cb_
    write_complete_cb_ = cb;
}

void TcpConnection::SetWriteCompleteCallback(WriteCompleteCallback &&cb)
{
    // 使用 std::move 将右值引用的回调函数移动到成员变量 write_complete_cb_
    write_complete_cb_ = std::move(cb);
}

void TcpConnection::Send(std::list<BufferNodePtr>&list)
{
    // 在事件循环中调用 SendInLoop 函数，传递数据列表
    loop_->RunInLoop([this, &list](){
        SendInLoop(list);
    });
}

void TcpConnection::Send(const char *buff, size_t size)
{
    // 在事件循环中调用 SendInLoop 函数，传递缓冲区和大小
    loop_->RunInLoop([this, buff, size](){
        SendInLoop(buff, size);
    });
}

void TcpConnection::SendInLoop(const char *buff, size_t size)
{
    // 检查连接是否已关闭
    if (closed_)
    {
        // 记录日志，显示对端地址已关闭连接
        NETWORK_TRACE << " host : " << peer_addr_.ToIpPort() << " had closed.";
        // 直接返回，不再处理
        return;
    }

    // 初始化一个变量 send_len 用于存储实际发送的字节数
    size_t send_len = 0;

    // 检查 io_vec_list_ 是否为空，如果为空，表示没有待发送的数据
    if (io_vec_list_.empty())
    {
        // 调用系统的 write 函数，将数据从 buff 发送到文件描述符 fd_，并将返回的字节数存储在 send_len 中
        send_len = ::write(fd_, buff, size);

        // 检查 send_len 是否小于 0，表示写入失败
        if (send_len < 0)
        {
            // 检查错误码 errno，如果不是中断、暂时不可用或阻塞错误，记录错误日志并调用 OnClose() 关闭连接
            if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                NETWORK_ERROR << " host : " << peer_addr_.ToIpPort() << " write err : " << errno;
                OnClose();

                return;
            }

            // 如果写入失败但错误是可恢复的，将 send_len 设置为 0
            send_len = 0;
        }

        // 从待发送的字节数中减去已成功发送的字节数
        size -= send_len;

        // 检查 size 是否为 0，通常表示没有数据需要写入
        if (size == 0)
        {
            // 如果 write_complete_cb_（写完成的回调函数）存在，则执行以下操作
            if (write_complete_cb_)
            {
                // 使用 std::dynamic_pointer_cast 将当前对象（通过 shared_from_this() 获取）转换为 TcpConnection 类型的智能指针，并调用回调函数
                write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
            }

            // 结束当前函数的执行
            return;
        }
    }

    // 如果还有未发送的数据
    if (size > 0)
    {
        // 创建一个 iovec 结构体
        struct iovec vec;
        // 设置 iov_base 为 buff 的偏移地址，指向未发送的数据
        vec.iov_base = (void*)(buff + send_len);
        // 设置 iov_len 为剩余的待发送字节数
        vec.iov_len = size;
        
        // 将 iovec 结构体添加到 io_vec_list_ 中，准备后续发送
        io_vec_list_.push_back(vec);

        // 调用 EnableWriting 函数，启用写入操作
        EnableWriting(true);
    }
}

void TcpConnection::SendInLoop(std::list<BufferNodePtr>&list)
{
    // 检查连接是否已关闭。如果是，记录日志并返回
    if (closed_)
    {
        NETWORK_TRACE << " host : " << peer_addr_.ToIpPort() << " had closed.";
        return;
    }

    // 遍历传入的 BufferNodePtr 列表
    for (auto &l : list)
    {
        // 定义一个 iovec 结构体
        struct iovec vec;
        // 设置 iov_base 为当前 BufferNodePtr 的地址
        vec.iov_base = (void*)l->addr;
        // 设置 iov_len 为当前 BufferNodePtr 的大小
        vec.iov_len = l->size;
        
        // 将 iovec 结构体添加到 io_vec_list_ 中
        io_vec_list_.push_back(vec);
    }

    // 如果 io_vec_list_ 不为空，调用 EnableWriting(true); 启用写入操作
    if (!io_vec_list_.empty())
    {
        EnableWriting(true);
    }
}

void TcpConnection::OnTimeout()
{
    NETWORK_ERROR << " host : " << peer_addr_.ToIpPort() << " timeout and close it.";
    std::cout << "host : " << peer_addr_.ToIpPort() << " timeout and close it." << std::endl;
    OnClose();
}

void TcpConnection::EnableCheckIdleTimeout(int32_t max_time)
{
    auto tp = std::make_shared<TimeoutEntry>(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
    max_idle_time_ = max_time;
    timeout_entry_ = tp;
    loop_->InsertEntry(max_time, tp);
}

void TcpConnection::SetTimeoutCallback(int timeout, const TimeoutCallback &cb)
{
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());
    loop_->RunAfter(timeout, [&cp, &cb](){
        cb(cp);
    });
}

void TcpConnection::SetTimeoutCallback(int timeout, TimeoutCallback &&cb)
{
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());
    loop_->RunAfter(timeout, [&cp, cb](){
        cb(cp);
    });
}

void TcpConnection::ExtendLife()
{
    auto tp = timeout_entry_.lock();

    if (tp)
    {
        loop_->InsertEntry(max_idle_time_, tp);
    }
}

TcpConnection::~TcpConnection()
{
    // 处理连接关闭的逻辑
    OnClose();
}