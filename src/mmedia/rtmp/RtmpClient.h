#pragma once
#include <functional>
#include <memory>
#include "network/TcpClient.h"
#include "network/net/EventLoop.h"
#include "network/base/InetAddress.h"
#include "mmedia/rtmp/RtmpHandler.h"

namespace lss
{
    namespace mm
    {
        using namespace lss::network;

        // 定义一个共享指针类型TcpClientPtr，指向TcpClient对象。
        using TcpClientPtr = std::shared_ptr<TcpClient>;

        class RtmpClient
        {
        public:
            // 构造函数，接受一个指向EventLoop对象的指针和一个指向RtmpHandler对象的指针，用于初始化RtmpClient对象
            RtmpClient(EventLoop *loop, RtmpHandler *handler);
            
            // 设置连接关闭的回调函数，传入一个常量引用的CloseConnectionCallback类型的回调函数
            void SetCloseCallback(const CloseConnectionCallback &cb);

            // 设置连接关闭的回调函数，使用右值引用的方式传递回调函数，支持移动语义
            void SetCloseCallback(CloseConnectionCallback &&cb);

            // 开始播放指定URL的RTMP流
            void Play(const std::string &url);

            // 开始发布指定URL的RTMP流
            void Publish(const std::string &url);

            // 发送RTMP数据包，使用右值引用接收数据包，支持移动语义
            void Send(PacketPtr &&data);

            // 析构函数，释放资源
            ~RtmpClient();

        private:  
            // 当写操作完成时的回调函数
            void OnWriteComplete(const TcpConnectionPtr &conn);

            // 处理连接建立或断开的回调函数，根据connected参数判断当前连接状态
            void OnConnection(const TcpConnectionPtr& conn, bool connected);

            // 处理接收到消息的回调函数，读取Tcp连接中的数据
            void OnMessage(const TcpConnectionPtr& conn, MsgBuffer &buff);  

            // 解析RTMP URL，返回是否解析成功
            bool ParseUrl(const std::string &url);

            // 创建TCP客户端，初始化网络连接
            void CreateTcpClient();  

            // 指向事件循环的指针，用于处理异步事件
            EventLoop *loop_{nullptr};

            // 存储网络地址信息，包含IP地址和端口号
            InetAddress addr_;

            // 指向RTMP协议处理器的指针，用于处理RTMP协议相关的逻辑
            RtmpHandler *handler_{nullptr};

            // TcpClient的共享指针，管理TCP连接的生命周期
            TcpClientPtr tcp_client_;

            // 保存RTMP流的URL
            std::string url_;

            // 表示当前是否为播放器模式，如果为true，则为播放模式；否则为发布模式
            bool is_player_{false};

            // 存储关闭连接时的回调函数
            CloseConnectionCallback close_cb_;
        };
    }
}