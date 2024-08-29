#include "TcpServer.h"
#include "network/base/Network.h"

using namespace lss::network;

// TcpServer构造函数，初始化事件循环和地址
TcpServer::TcpServer(EventLoop *loop, const InetAddress &addr)
    : loop_(loop)       // 将事件循环指针赋值给成员变量loop_
    , addr_(addr)       // 将地址赋值给成员变量addr_
{
    // 创建一个Acceptor对象，用于接受连接
    acceptor_ = std::make_shared<Acceptor>(loop, addr);
}

void TcpServer::SetNewConnectionCallback(const NewConnectionCallback &cb)
{
    // 将回调函数赋值给成员变量
    new_connection_cb_ = cb;
}

void TcpServer::SetNewConnectionCallback(NewConnectionCallback &&cb)
{
    // 将回调函数赋值给成员变量，使用移动语义赋值
    new_connection_cb_ = std::move(cb);
}

void TcpServer::SetDestroyConnectionCallback(const DestroyConnectionCallback &cb)
{
    // 将回调函数赋值给成员变量
    destroy_connection_cb_ = cb;
}

void TcpServer::SetDestroyConnectionCallback(DestroyConnectionCallback &&cb)
{
    // 将回调函数赋值给成员变量，使用移动语义赋值
    destroy_connection_cb_ = std::move(cb);
}

void TcpServer::OnAccept(int fd, const InetAddress &addr)
{
    // 记录新连接信息
    NETWORK_TRACE << " new connection fd : " << fd << " host : " << addr.ToIpPort();

    // 创建TcpConnection对象
    TcpConnectionPtr con = std::make_shared<TcpConnection>(loop_, fd, addr_, addr);
    // 设置连接关闭时的回调
    con->SetCloseCallback(std::bind(&TcpServer::OnConnectionClose, this, std::placeholders::_1));
    
    if (write_complete_cb_)
    {
        // 设置写操作完成时的回调
        con->SetWriteCompleteCallback(write_complete_cb_);
    }

    if (active_cb_)
    {
        // 设置激活完成时的回调
        con->SetActiveCallback(active_cb_);
    }

    // 设置接收消息的回调
    con->SetRecvMsgCallback(message_cb_);
    // 将连接插入到连接集合中
    connections_.insert(con);
    // 将连接添加到事件循环中
    loop_->AddEvent(con);
    // 启用空闲超时检查，单位：秒
    con->EnableCheckIdleTimeout(30);

    if (new_connection_cb_)
    {
        // 调用新的连接回调
        new_connection_cb_(con);
    }
}

void TcpServer::OnConnectionClose(const TcpConnectionPtr &con)
{
    // 记录关闭信息
    NETWORK_TRACE << " host : " << con->PeerAddr().ToIpPort() << " closed.";

    // 确保在事件循环线程中执行
    loop_->AssertInLoopThread();
    // 从连接集合中移除连接
    connections_.erase(con);
    // 从事件循环中删除连接
    loop_->DelEvent(con);

    if (destroy_connection_cb_)
    {
        // 调用连接销毁回调
        destroy_connection_cb_(con);
    }
}

void TcpServer::SetActiveCallback(const ActiveCallback &cb)
{
    // 将回调函数赋值给成员变量
    active_cb_ = cb;
}

void TcpServer::SetActiveCallback(ActiveCallback &&cb)
{
    // 将回调函数赋值给成员变量，使用移动语义赋值
    active_cb_ = std::move(cb);
}

void TcpServer::SetWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    // 将回调函数赋值给成员变量
    write_complete_cb_ = cb;
}

void TcpServer::SetWriteCompleteCallback(WriteCompleteCallback &&cb)
{
    // 将回调函数赋值给成员变量，使用移动语义赋值
    write_complete_cb_ = std::move(cb);
}

void TcpServer::SetMessageCallback(const MessageCallback &cb)
{
    // 将回调函数赋值给成员变量
    message_cb_ = cb;
}

void TcpServer::SetMessageCallback(MessageCallback &&cb)
{
    // 将回调函数赋值给成员变量，使用移动语义赋值
    message_cb_ = std::move(cb);
}

void TcpServer::Start()
{
    // 设置接受连接的回调
    acceptor_->SetAcceptCallback(std::bind(&TcpServer::OnAccept, this, std::placeholders::_1, std::placeholders::_2));
    // 启动Acceptor
    acceptor_->Start();
}

void TcpServer::Stop()
{
    // 停止Acceptor
    acceptor_->Stop();
}

TcpServer::~TcpServer()
{

}