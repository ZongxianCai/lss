#pragma once
#include "mmedia/rtmp/RtmpHandler.h"
#include "network/net/TcpConnection.h"
#include "network/TcpServer.h"

namespace lss
{
    namespace mm
    {
        using namespace lss::network;

        class RtmpServer : public TcpServer
        {
        public:
            // 构造函数，接收 EventLoop 指针、InetAddress 引用和一个可选的 RtmpHandler 指针
            RtmpServer(EventLoop *loop,const InetAddress &local, RtmpHandler *handler = nullptr);

            // 启动服务器，重写父类的 Start 方法
            void Start() override;

            // 停止服务器，重写父类的 Stop 方法
            void Stop() override;

            // 析构函数，清理资源
            ~RtmpServer();

        private:
            // 新连接到来时的回调函数
            void OnNewConnection(const TcpConnectionPtr &conn);

            // 连接被销毁时的回调函数
            void OnDestroyed(const TcpConnectionPtr &conn);

            // 接收到消息时的回调函数
            void OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buff);

            // 写操作完成时的回调函数
            void OnWriteComplete(const ConnectionPtr &conn);

            // 连接被激活时的回调函数
            void OnActive(const ConnectionPtr &conn);

            // RtmpHandler 指针，用于处理 RTMP 相关的逻辑
            RtmpHandler *rtmp_handler_{nullptr};
        };
    }
}