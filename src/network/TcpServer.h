#pragma once
#include <functional>
#include <memory>
#include <unordered_set>
#include "network/net/TcpConnection.h"
#include "network/net/EventLoop.h"
#include "network/net/Acceptor.h"
#include "network/base/InetAddress.h"

namespace lss
{
    namespace network
    {
        // 定义一个新连接回调类型，接受一个 TcpConnectionPtr 类型的参数
        using NewConnectionCallback = std::function<void (const TcpConnectionPtr &)>;

        // 定义一个连接销毁回调类型，接受一个 TcpConnectionPtr 类型的参数
        using DestroyConnectionCallback = std::function<void (const TcpConnectionPtr &)>;
        
        // TcpServer 类的定义
        class TcpServer
        {
        public:
            // 构造函数，接受事件循环和地址作为参数
            TcpServer(EventLoop *loop, const InetAddress &addr);

            // 设置新连接回调函数（左值引用）
            void SetNewConnectionCallback(const NewConnectionCallback &cb);

            // 设置新连接回调函数（右值引用）
            void SetNewConnectionCallback(NewConnectionCallback &&cb);

            // 设置连接销毁回调函数（左值引用）
            void SetDestroyConnectionCallback(const DestroyConnectionCallback &cb);

            // 设置连接销毁回调函数（右值引用）
            void SetDestroyConnectionCallback(DestroyConnectionCallback &&cb);

            // 处理新连接的函数，接受文件描述符和地址作为参数
            void OnAccept(int fd, const InetAddress &addr);

            // 处理连接关闭的函数，接受一个 TcpConnectionPtr 类型的参数
            void OnConnectionClose(const TcpConnectionPtr &con);

            // 设置激活回调函数（左值引用）
            void SetActiveCallback(const ActiveCallback &cb);

            // 设置激活回调函数（右值引用）
            void SetActiveCallback(ActiveCallback &&cb);

            // 设置写完成回调函数（左值引用）
            void SetWriteCompleteCallback(const WriteCompleteCallback &cb);

            // 设置写完成回调函数（右值引用）
            void SetWriteCompleteCallback(WriteCompleteCallback &&cb);

            // 设置消息回调函数（左值引用）
            void SetMessageCallback(const MessageCallback &cb);

            // 设置消息回调函数（右值引用）
            void SetMessageCallback(MessageCallback &&cb);

            // 启动服务器的虚函数
            virtual void Start();

            // 停止服务器的虚函数
            virtual void Stop();

            // 析构函数（虚函数，允许子类重写以实现特定的启动和停止逻辑）
            virtual ~TcpServer();

        private:
            // 事件循环指针
            EventLoop *loop_{nullptr};

            // 服务器地址
            InetAddress addr_;

            // 接受器的智能指针
            std::shared_ptr<Acceptor> acceptor_;

            // 新连接回调函数
            NewConnectionCallback new_connection_cb_;

            // 当前连接的集合
            std::unordered_set<TcpConnectionPtr> connections_;

            // 消息回调函数
            MessageCallback message_cb_;

            // 激活回调函数
            ActiveCallback active_cb_;

            // 写完成回调函数
            WriteCompleteCallback write_complete_cb_;

            // 连接销毁回调函数
            DestroyConnectionCallback destroy_connection_cb_;
        };
    }
}