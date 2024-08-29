#pragma once
    
#include <functional>
#include <memory>
#include <list>
#include <sys/uio.h>
#include "Connection.h"
#include "network/base/InetAddress.h"
#include "network/base/MsgBuffer.h"

namespace lss
{
    namespace network
    {
        // 前置声明，避免循环依赖
        class TcpConnection;

        // 定义 TcpConnection 的智能指针类型，是 TcpConnection 类的智能指针，使用智能指针可以自动管理内存，避免内存泄漏
        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

        // 定义关闭连接的回调函数类型，接受一个 TcpConnectionPtr 类型的参数，返回类型为 void，用于处理连接关闭事件
        using CloseConnectionCallback = std::function<void(const TcpConnectionPtr &)>;

        // 定义消息回调函数类型，接受一个 TcpConnectionPtr 和一个 MsgBuffer 类型的参数，用于处理接收到的消息
        using MessageCallback = std::function<void(const TcpConnectionPtr &, MsgBuffer &buffer)>;

        // 定义写入完成的回调函数类型，接受一个 TcpConnectionPtr 参数，用于处理写入完成事件
        using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;

        // 定义超时回调函数类型，接受一个 TcpConnectionPtr 参数，用于处理超时事件
        using TimeoutCallback = std::function<void(const TcpConnectionPtr &)>;

        // 定义一个缓冲区节点
        struct BufferNode
        {
            BufferNode(void *buff, size_t s)
                : addr(buff), size(s)
            {}

            void *addr{nullptr};        // 指向缓冲区的起始地址

            size_t size{0};             // 缓冲区的大小
        };

        // 定义一个缓冲区节点 BufferNode 的智能指针，便于管理 BufferNode 的内存
        using BufferNodePtr = std::shared_ptr<BufferNode>;

        // 前置声明，避免循环依赖
        struct TimeoutEntry;

        class TcpConnection : public Connection // 继承自 Connection 类，包含与 TCP 连接相关的功能和数据成员
        {
        public:
            // 构造函数，初始化事件循环、套接字和地址
            TcpConnection(EventLoop *loop, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr);

            // 设置关闭连接的回调函数
            void SetCloseCallback(const CloseConnectionCallback &cb);

            // 设置关闭连接的回调函数（右值引用）
            void SetCloseCallback(CloseConnectionCallback &&cb);

            // 关闭连接时调用的函数
            void OnClose() override;

            // 强制关闭连接的函数
            void ForceClose() override;

            // 读取数据时调用的函数
            void OnRead() override;

            // 设置接收消息的回调函数
            void SetRecvMsgCallback(const MessageCallback &cb);

            // 设置接收消息的回调函数（右值引用）
            void SetRecvMsgCallback(MessageCallback &&cb);

            // 发生错误时调用的函数
            void OnError(const std::string &msg) override;

            // 写入数据时调用的函数
            void OnWrite() override;

            // 设置写入完成的回调函数
            void SetWriteCompleteCallback(const WriteCompleteCallback &cb);

            // 设置写入完成的回调函数（右值引用）
            void SetWriteCompleteCallback(WriteCompleteCallback &&cb);

            // 发送数据列表的函数
            void Send(std::list<BufferNodePtr>&list);

            // 发送指定大小的缓冲区数据的函数
            void Send(const char *buff, size_t size);

            // 设置超时时间
            void OnTimeout();

            // 设置最大空闲时间
            void EnableCheckIdleTimeout(int32_t max_time);

            // 设置超时回调函数
            void SetTimeoutCallback(int timeout, const TimeoutCallback &cb);

            // 设置超时回调函数（右值引用）
            void SetTimeoutCallback(int timeout, TimeoutCallback &&cb);

            // 析构函数
            virtual ~TcpConnection();

        private:
            // 在事件循环中发送数据的函数
            void SendInLoop(const char *buff, size_t size);

            // 在事件循环中发送数据列表的函数
            void SendInLoop(std::list<BufferNodePtr>&list);

            // 在事件循环中发送指定大小的缓冲区数据的函数
            void ExtendLife();

            // 连接是否关闭的标志
            bool closed_{false};

            // 关闭连接时的回调函数
            CloseConnectionCallback close_cb_;

            // 存储接收到的消息的缓冲区
            MsgBuffer message_buffer_;

            // 接收消息时的回调函数
            MessageCallback message_cb_; 

            // 存储写入事件的数据队列(内存空间连续)
            std::vector<struct iovec> io_vec_list_;

            // 写入完成时的回调函数
            WriteCompleteCallback write_complete_cb_;

            // 声明了一个指向 TimeoutEntry 对象的弱指针，使用弱指针可以避免循环引用，从而防止内存泄漏
            // 弱指针不会管理所指向对象的生命周期，因此可以指向由 std::shared_ptr 管理的对象，而不会阻止该对象被销毁
            std::weak_ptr<TimeoutEntry> timeout_entry_;

            // 连接的最大空闲时间，单位:秒
            int32_t max_idle_time_{30};
        };

        // 定义一个超时时间节点
        struct TimeoutEntry
        {
            // 构造函数接受一个 TcpConnectionPtr 类型的常量引用 c（一个智能指针，指向 TcpConnection 对象）
            TimeoutEntry(const TcpConnectionPtr &c)
                : conn(c) // 使用初始化列表将 conn 成员初始化为传入的 c
            {}

            ~TimeoutEntry()
            {
                // 尝试将 conn 转换为一个 std::shared_ptr<TcpConnection>，如果 conn 指向的对象仍然存在（即没有被销毁），lock() 方法将返回一个有效的 shared_ptr
                auto c = conn.lock();

                // 如果 c 是有效的，则调用 c->OnTimeout(); 
                if (c)
                {
                    // 在超时发生时，相关的 TcpConnection 对象将执行其 OnTimeout 方法
                    c->OnTimeout();
                }
            }

            // conn 是一个 std::weak_ptr<TcpConnection> 类型的成员变量，用于存储指向 TcpConnection 对象的弱引用
            // 使用弱指针可以避免循环引用，从而防止内存泄漏。
            std::weak_ptr<TcpConnection> conn;
        };
    }
}