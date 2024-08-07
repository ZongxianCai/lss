#include "Acceptor.h"
#include "network/base/Network.h"

using namespace lss::network;

// Acceptor 类的构造函数，接收 EventLoop 指针和 InetAddress 引用作为参数
Acceptor::Acceptor(EventLoop *loop, const InetAddress &addr)
: Event(loop)       // 调用基类 Event 的构造函数，传入 loop 指针
, addr_(addr)       // 初始化成员变量 addr_ 为传入的 addr 参数
{

}

// 设置接收连接的回调函数，接收一个 AcceptCallback 类型的常引用参数
void Acceptor::SetAcceptCallback(const AcceptCallback &cb)
{
    // 将传入的回调函数赋值给 accept_cb_
    accept_cb_ = cb;
}

// 设置接受连接的回调函数，接受一个AcceptCallback类型的右值引用参数
void Acceptor::SetAcceptCallback(AcceptCallback &&cb)
{
    // 使用std::move将传入的回调函数移动赋值给accept_cb_
    accept_cb_ = std::move(cb);
}

// 开始接收连接
void Acceptor::Start()
{
    // 在 EventLoop 中运行 lambda 表达式
    loop_->RunInLoop([this](){
        Open();    // 调用Open函数，打开套接字
    });
}

// 停止接收连接
void Acceptor::Stop()
{
    // 从EventLoop 中删除 Acceptor 事件
    loop_->DelEvent(std::dynamic_pointer_cast<Acceptor>(shared_from_this()));
}

// 处理读取事件
void Acceptor::OnRead()
{
    // 检查 socket_opt_ 是否有效
    if (!socket_opt_)
    {
        // 如果无效，直接返回
        return;
    }

    // 开始一个无限循环
    while (true)
    {
        // 创建一个 InetAddress 对象，用于存储接受到的连接的地址
        InetAddress addr;
        // 调用 socket_opt_ 的 Accept 方法，接受连接并获取连接的 socket 文件描述符
        auto sock = socket_opt_->Accept(&addr);

        // 检查 socket 是否有效
        if (sock >= 0)
        {
            // 如果设置了接受回调函数
            if (accept_cb_)
            {
                // 调用回调函数，传入 socket 和远端地址
                accept_cb_(sock, addr);
            }
        }
        else // 如果接受连接失败
        {
            // 检查错误码是否为中断或暂时不可用
            if (errno != EINTR && errno != EAGAIN)
            {
                // 记录网络错误信息
                NETWORK_ERROR << " acceptor error. errno : " << errno;
                // 调用 OnClose 函数处理关闭逻辑
                OnClose();
            }
            // 退出循环
            break;
        }
    }
}

// 处理错误的函数，接受一个错误信息字符串
void Acceptor::OnError(const std::string &msg)
{
    // 记录错误信息
    NETWORK_ERROR << " acceptor error : " << msg;
    // 调用 OnClose 函数处理关闭逻辑
    OnClose();
}

// 处理关闭事件
void Acceptor::OnClose()
{
    // 停止接收连接
    Stop();
    // 重新开启接收套接字
    Open();
}

// 开启接收套接字
void Acceptor::Open()
{
    // 如果文件描述符大于 0 ，表示已有打开的 socket
    if (fd_ > 0)
    {
        // 关闭当前的 socket
        ::close(fd_);
        // 将文件描述符设置为 -1 ，表示无效
        fd_ = -1;
    }

    // 检查地址是否为 IPv6
    if (addr_.IsIpV6())
    {
        // 创建一个非阻塞的 IPv6 TCP socket
        fd_ = SocketOpt::CreateNonblockingTcpSocket(AF_INET6);
    }
    else
    {
        // 创建一个非阻塞的 IPv4 TCP socket
        fd_ = SocketOpt::CreateNonblockingTcpSocket(AF_INET);
    }

    // 检查 socket 创建是否成功
    if (fd_ < 0)
    {
        // 输出错误信息并退出程序
        NETWORK_ERROR << " socket failed. errno : " << errno;
        exit(-1);
    }

    // 如果 socket_opt_ 已存在
    if (socket_opt_)
    {
        // 删除
        delete socket_opt_;
        // 将指针置为 nullptr
        socket_opt_ = nullptr;
    }

    // 将当前接受器添加到事件循环中
    loop_->AddEvent(std::dynamic_pointer_cast<Acceptor>(shared_from_this()));
    // 创建新的 SocketOpt 对象
    socket_opt_ = new SocketOpt(fd_);
    // 设置 socket 选项，允许地址重用
    socket_opt_->SetReuseAddr(true);
    // 设置 socket 选项，允许端口重用
    socket_opt_->SetReusePort(true);
    // 绑定地址到 socket
    socket_opt_->BindAddress(addr_);
    // 开始监听连接
    socket_opt_->Listen();

}

// 析构函数，释放资源
Acceptor::~Acceptor()
{
    // 停止接收连接
    Stop();

    // 如果 socket_opt_ 指针不为空
    if (socket_opt_)
    {
        // 删除 socket_opt_ 指针指向的对象
        delete socket_opt_;
        // 将 socket_opt_ 指针置为 nullptr
        socket_opt_ = nullptr;
    }
}