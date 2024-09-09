#include "RtmpClient.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/rtmp/RtmpContext.h"

using namespace lss::mm;
using namespace lss::network;

RtmpClient::RtmpClient(EventLoop *loop, RtmpHandler *handler)
    : loop_(loop)               // 初始化列表初始化成员变量 loop_ ，指向事件循环
    , handler_(handler)         // 初始化列表初始化成员变量 handler_ ，指向 RTMP 协议处理器
{

}

void RtmpClient::OnWriteComplete(const TcpConnectionPtr &conn)
{
    // 从连接对象中获取上下文对象，类型为 RtmpContext，使用预定义的键 kRtmpContext 进行查找
    auto context = conn->GetContext<RtmpContext>(kRtmpContext);

    // 如果上下文对象存在，调用其 OnWriteComplete 方法，处理写操作完成的后续逻辑
    if (context)
    {
        context->OnWriteComplete();
    }
}

void RtmpClient::OnConnection(const TcpConnectionPtr& conn, bool connected)
{
    // 如果连接已建立（connected 为 true），则创建一个 RtmpContext 对象，并用 shared_ptr 管理它
    if (connected)
    {
        // 该对象会存储连接（conn）、处理器（handler_）以及一个初始状态（true）
        auto context = std::make_shared<RtmpContext>(conn, handler_, true);

        // 如果当前对象是播放器模式，则调用 context 的 Play 方法，传入 URL，开始播放流媒体
        if (is_player_)
        {
            context->Play(url_);
        }
        // 否则，调用 Publish 方法，开始发布流媒体
        else 
        {
            context->Publish(url_);
        }

        // 将新创建的 context 设置为连接对象的上下文，方便后续操作访问
        conn->SetContext(kRtmpContext, context);

        // 开始RTMP握手流程，建立与服务器的通信
        context->StartHandShake();
    }
}

void RtmpClient::OnMessage(const TcpConnectionPtr& conn, MsgBuffer &buf)
{
    // 从连接对象中获取与该连接关联的上下文对象，类型为 RtmpContext，使用键 kRtmpContext 查找
    auto context = conn->GetContext<RtmpContext>(kRtmpContext);

    // 如果上下文对象存在，调用其 Parse 方法，传入接收到的数据缓冲区 buf，进行消息解析
    if (context)
    {
        // Parse 方法返回一个整数，表示解析结果
        auto ret = context->Parse(buf);

        // 如果解析结果为 -1，表示解析失败，记录错误日志
        if (ret == -1)
        {
            RTMP_ERROR << " message parse error.";

            // 强制关闭连接，避免错误继续传播
            conn->ForceClose();
        }
    }
}

void RtmpClient::SetCloseCallback(const CloseConnectionCallback &cb)
{
    // 设置关闭连接的回调函数，传入一个常量引用的 CloseConnectionCallback 类型的回调函数
    close_cb_ = cb;
}

void RtmpClient::SetCloseCallback(CloseConnectionCallback &&cb)
{
    // 设置关闭连接的回调函数，使用右值引用，将传入的回调函数移动到成员变量 close_cb_ 中，避免不必要的拷贝
    close_cb_ = std::move(cb);
}

void RtmpClient::Play(const std::string &url)
{
    // 设置当前对象为播放器模式
    is_player_ = true;

    // 保存RTMP流的URL
    url_ = url;

    // 调用 CreateTcpClient 函数，创建并初始化TCP客户端，开始与RTMP服务器的连接
    CreateTcpClient();
}

void RtmpClient::Publish(const std::string &url)
{
    // 设置当前对象为发布者模式
    is_player_ = false;

    // 保存RTMP流的URL
    url_ = url;

    // 调用 CreateTcpClient 函数，创建并初始化TCP客户端，开始与RTMP服务器的连接
    CreateTcpClient();
}

bool RtmpClient::ParseUrl(const std::string &url)
{
    // 检查URL的长度是否大于7，确保包含至少 "rtmp://" 前缀
    if (url.size() > 7) 
    {
        // 初始化默认端口1935（RTMP协议的标准端口）
        uint16_t port = 1935;

        // 从索引7开始查找':'或'/'字符的位置，用于分割域名和端口或路径
        auto pos = url.find_first_of(":/", 7);

        // 如果找到了':'或'/'字符
        if (pos != std::string::npos)
        {
            // 提取域名部分，从第7个字符到pos的位置
            std::string domain = url.substr(7, pos - 7);

            // 检查pos位置的字符是否为':'，表示后面可能跟随端口号
            if (url.at(pos) == ':')
            {
                // 查找从pos+1开始的第一个'/'的位置，用于确定端口号的结尾
                auto pos1 = url.find_first_of("/", pos + 1);

                // 如果找到了'/'，提取端口号并转换为整数
                if (pos1 != std::string::npos)
                {
                    // 提取':'和'/'之间的子字符串，转换为端口号
                    port = std::atoi(url.substr(pos + 1, pos1 - pos).c_str());
                }
            }

            // 设置InetAddress对象的域名或IP地址
            addr_.SetAddr(domain);

            // 设置InetAddress对象的端口号
            addr_.SetPort(port);

            // 返回true，表示URL解析成功
            return true;
        }
    }

    // 如果URL格式不符合要求，返回false，表示解析失败
    return false;
}

void RtmpClient::CreateTcpClient()
{
    // 调用 ParseUrl 函数解析 URL，并将返回结果存储在 ret 变量中
    auto ret = ParseUrl(url_);

    // 如果 URL 解析失败（ret 为 false）
    if (!ret)
    {
        // 打印错误日志，提示 URL 无效
        RTMP_ERROR << " invalid url : " << url_;

        // 如果设置了关闭回调函数（close_cb_ 非空），则调用该回调，传入 nullptr，表示连接关闭
        if (close_cb_)
        {
            close_cb_(nullptr);
        }

        // 终止函数执行，返回
        return;
    }

    // 创建一个新的 TcpClient 对象，并使用 shared_ptr 管理，将事件循环 (loop_) 和解析出的地址 (addr_) 传递给 TcpClient 构造函数
    tcp_client_ = std::make_shared<TcpClient>(loop_, addr_);

    // 设置写完成回调函数，当数据写入完成时，将调用 RtmpClient::OnWriteComplete 方法，std::bind 用于将成员函数绑定到当前对象实例上
    tcp_client_->SetWriteCompleteCallback(std::bind(&RtmpClient::OnWriteComplete, this, std::placeholders::_1));

    // 设置消息接收回调函数，当接收到数据时，将调用 RtmpClient::OnMessage 方法，并传递连接对象和消息缓冲区
    tcp_client_->SetRecvMsgCallback(std::bind(&RtmpClient::OnMessage, this, std::placeholders::_1, std::placeholders::_2));

    // 设置关闭回调函数，当连接关闭时调用 previously provided close callback (close_cb_)
    tcp_client_->SetCloseCallback(close_cb_);

    // 设置连接回调函数，当建立连接时，将调用 RtmpClient::OnConnection 方法，传递连接对象和连接状态
    tcp_client_->SetConnectCallback(std::bind(&RtmpClient::OnConnection, this, std::placeholders::_1, std::placeholders::_2));

    // 调用 Connect 方法，启动与服务器的连接
    tcp_client_->Connect();
}

RtmpClient::~RtmpClient()
{

}