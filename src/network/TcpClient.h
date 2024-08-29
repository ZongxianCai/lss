#pragma once
#include <functional>
#include "network/base/InetAddress.h"
#include "network/net/TcpConnection.h"
#include "network/net/EventLoop.h"

namespace lss
{
    namespace network
    {
        // 定义 TCP 连接状态的枚举
        enum
        {   
            kTcpConStatusInit = 0,              // 初始化状态
            kTcpConStatusConnecting = 1,        // 连接中状态
            kTcpConStatusConnected = 2,         // 已连接状态
            kTcpConStatusDisConnected = 3       // 断开连接状态
        };

        // 定义连接回调类型，用于处理连接状态的回调
        using ConnectionCallback = std::function<void (const TcpConnectionPtr &con, bool)>;

        // TcpClient 类，继承自 TcpConnection
        class TcpClient : public TcpConnection
        {
        public:
            // TcpClient构造函数，初始化TcpConnection并设置服务器地址
            TcpClient(EventLoop *loop, const InetAddress &server);

             // 连接方法
            void Connect();

            // 设置连接回调（左值引用）
            void SetConnectCallback(const ConnectionCallback &cb);

            // 设置连接回调（右值引用）
            void SetConnectCallback(ConnectionCallback &&cb);

            // 重写读取事件处理
            void OnRead() override;

            // 重写写入事件处理
            void OnWrite() override;

            // 重写关闭事件处理
            void OnClose() override;  

            // 发送数据（缓冲区列表）
            void Send(std::list<BufferNodePtr> &list);

            // 发送数据（原始数据）
            void Send(const char *buff, size_t size);          

            // 析构函数
            virtual ~TcpClient();

        private:
            // 在事件循环中进行连接
            void ConnectInLoop();

            // 更新连接状态
            void UpdateConnectionStatus();

            // 检查错误
            bool CheckError();

            // 服务器地址
            InetAddress server_addr_;

            // 连接状态，初始为 kTcpConStatusInit
            int32_t status_{kTcpConStatusInit};

            // 连接回调
            ConnectionCallback connected_cb_;
        };
    }
}