#include "RtmpServer.h"
#include "mmedia/base/MMediaLog.h"
#include "RtmpHandShake.h"

using namespace lss::mm;

// RtmpServer 构造函数，初始化基类 TcpServer 和 rtmp_handler_ 成员变量
RtmpServer::RtmpServer(EventLoop *loop, const InetAddress &local, RtmpHandler *handler)
    : TcpServer(loop, local)        // 调用 TcpServer 构造函数，初始化事件循环和本地地址
    , rtmp_handler_(handler)        // 初始化 rtmp_handler_，如果传入了 handler 则设置为该值
{

}

void RtmpServer::Start()
{
    // 设置新连接回调函数，当有新连接时调用 OnNewConnection
    TcpServer::SetNewConnectionCallback(std::bind(&RtmpServer::OnNewConnection, this, std::placeholders::_1));

    // 设置连接销毁回调函数，当连接销毁时调用 OnDestroyed
    TcpServer::SetDestroyConnectionCallback(std::bind(&RtmpServer::OnDestroyed, this, std::placeholders::_1));

    // 设置消息接收回调函数，当接收到消息时调用 OnMessage
    TcpServer::SetMessageCallback(std::bind(&RtmpServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2));

    // 设置写完成回调函数，当写操作完成时调用 OnWriteComplete
    TcpServer::SetWriteCompleteCallback(std::bind(&RtmpServer::OnWriteComplete, this, std::placeholders::_1));

    // 设置连接激活回调函数，当连接变为活跃时调用 OnActive
    TcpServer::SetActiveCallback(std::bind(&RtmpServer::OnActive, this, std::placeholders::_1));

    // 调用基类的 Start 方法，启动服务器
    TcpServer::Start();
}

void RtmpServer::Stop()
{
    // 调用基类的 Stop 方法，停止服务器
    TcpServer::Stop();
}

void RtmpServer::OnNewConnection(const TcpConnectionPtr &conn)
{
    // 如果 rtmp_handler_ 不为空，调用其 OnNewConnection 方法处理新连接
    if (rtmp_handler_)
    {
        rtmp_handler_->OnNewConnection(conn);
    }

    // 创建一个 RtmpHandShake 对象，用于处理 RTMP 握手流程
    RtmpHandShakePtr shake = std::make_shared<RtmpHandShake>(conn);
    
    // 将 RtmpHandShake 对象存储在连接的上下文中，方便后续使用
    conn->SetContext(kRtmpContext, shake);

    // 开始握手流程
    shake->Start();
}

void RtmpServer::OnDestroyed(const TcpConnectionPtr &conn)
{
    // 如果 rtmp_handler_ 不为空，调用其 OnConnectionDestroy 方法处理连接销毁
    if (rtmp_handler_)
    {
        rtmp_handler_->OnConnectionDestroy(conn);
    }

    // 清除连接的 RTMP 上下文，释放资源
    conn->ClearContext(kRtmpContext);
}

void RtmpServer::OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buff)
{
    // 从连接的上下文中获取 RtmpHandShake 对象
    RtmpHandShakePtr shake = conn->GetContext<RtmpHandShake>(kRtmpContext);

    // 如果握手对象存在，继续处理握手消息
    if (shake)
    {
        // 调用 HandShake 方法处理消息，返回值表示握手状态
        int ret = shake->HandShake(buff);

        // 如果握手成功，记录日志
        if (ret == 0)
        {
            RTMP_TRACE << " host : " << conn->PeerAddr().ToIpPort() << " handshake success.";
        }
        // 如果握手失败，强制关闭连接
        else if (ret == -1)
        {
            conn->ForceClose();
        }
    }
}

void RtmpServer::OnWriteComplete(const ConnectionPtr &conn)
{
    // 从连接的上下文中获取 RtmpHandShake 对象
    RtmpHandShakePtr shake = conn->GetContext<RtmpHandShake>(kRtmpContext);

    // 如果握手对象存在，调用 WriteComplete 方法处理写完成逻辑
    if (shake)
    {
        shake->WriteComplete();
    }
}

void RtmpServer::OnActive(const ConnectionPtr &conn)
{
    // 如果 rtmp_handler_ 不为空，调用其 OnActive 方法处理连接激活
    if (rtmp_handler_)
    {
        rtmp_handler_->OnActive(conn);
    }
}

RtmpServer::~RtmpServer()
{
    // 停止服务器
    Stop();
}