#pragma once
#include <list>
#include <functional>
#include <memory>
#include "network/base/InetAddress.h"
#include "network/base/MsgBuffer.h"
#include "network/net/EventLoop.h"
#include "network/net/Connection.h"

namespace lss
{
    namespace network
    {
        // 前向声明UdpSocket类
        class UdpSocket;

        // 定义UdpSocket的智能指针类型，用于管理UdpSocket对象的生命周期
        using UdpSocketPtr = std::shared_ptr<UdpSocket>;

        // 定义UdpSocket的智能指针类型，处理接收到的消息
        using UdpSocketMessageCallback = std::function<void (const InetAddress &addr, MsgBuffer &buff)>;

        // 定义UdpSocket的智能指针类型，处理写入完成事件
        using UdpSocketWriteCompleteCallback = std::function<void (const UdpSocketPtr &)>;

        // 定义UdpSocket的智能指针类型，处理连接关闭事件
        using UdpSocketCloseConnectionCallback = std::function<void(const UdpSocketPtr &)>;

        // 定义UdpSocket的智能指针类型，处理超时事件
        using UdpSocketTimeoutCallback = std::function<void(const UdpSocketPtr &)>;

        // 结构体前向声明
        struct UdpTimeoutEntry;

        // 定义UdpBufferNode类，继承自BufferNode，用于存储UDP数据包的信息
        struct UdpBufferNode : public BufferNode
        {
            UdpBufferNode(void *buff, size_t s, struct sockaddr *saddr, socklen_t len)
                : BufferNode(buff, s)
                , sock_addr(saddr)
                , sock_len(len)
            {}

            // 存储地址信息
            struct sockaddr *sock_addr{nullptr};
            // 存储地址长度
            socklen_t sock_len{0};
        };

        // 定义UdpBufferNodePtr的智能指针类型，用于管理UdpBufferNode对象的生命周期
        using UdpBufferNodePtr = std::shared_ptr<UdpBufferNode>;

        // UdpSocket类继承自Connection，用于管理UDP连接
        class UdpSocket : public Connection
        {
        public:
            // 构造函数接受一个事件循环指针、一个套接字文件描述符、一个本地地址和一个远端地址，用于初始化 UdpSocket 实例
            UdpSocket(EventLoop *loop, int socketfd, const InetAddress &localAddr, const InetAddress &peerAddr);

            // 设置关闭连接的回调函数
            void SetCloseCallback(const UdpSocketCloseConnectionCallback &cb);

            // 设置关闭连接的回调函数，接受右值引用，允许使用临时对象
            void SetCloseCallback(UdpSocketCloseConnectionCallback &&cb);

            // 设置接收到消息的回调函数
            void SetRecvMsgCallback(const UdpSocketMessageCallback &cb);

            // 设置接收到消息的回调函数，接受右值引用，允许使用临时对象
            void SetRecvMsgCallback(UdpSocketMessageCallback &&cb);

            // 设置写入完成的回调函数
            void SetWriteCompleteCallback(const UdpSocketWriteCompleteCallback &cb);

            // 设置写入完成的回调函数，接受右值引用，允许使用临时对象
            void SetWriteCompleteCallback(UdpSocketWriteCompleteCallback &&cb);

            // 设置超时的回调函数
            void SetTimeoutCallback(int timeout, const UdpSocketTimeoutCallback &cb);

            // 设置超时的回调函数，接受右值引用，允许使用临时对象
            void SetTimeoutCallback(int timeout, UdpSocketTimeoutCallback &&cb);

            // 超时处理
            void OnTimeOut();

            // 启用空闲超时检查，max_time指定最大空闲时间，单位：ms
            void EnableCheckIdleTimeout(int32_t max_time);

            // 发送数据，接受一个UdpBufferNodePtr类型的列表，表示要发送的数据包
            void Send(std::list<UdpBufferNodePtr> &list);

            // 重载的Send方法，允许直接发送一个字符缓冲区。buff是要发送的数据，size是数据的大小，addr是目标地址，len是地址的长度
            void Send(const char *buff, size_t size, struct sockaddr *addr, socklen_t len);
            
            // 重写错误事件的处理方法（从基类继承而来），msg 参数包含错误信息
            void OnError(const std::string &msg) override;

            // 重写读取事件的处理方法
            void OnRead() override;

            // 重写写入事件的处理方法
            void OnWrite() override;

            // 重写关闭事件的处理方法
            void OnClose() override;

            // 重写强制关闭事件的处理方法
            void ForceClose() override;

            // 析构函数
            ~UdpSocket();

        private:
            // 延长某个对象或连接的生命周期
            void ExtendLife();

            // 循环发送队列中的数据，表示要在循环中发送的数据包
            void SendInLoop(std::list<UdpBufferNodePtr> &list);

            // 方法重载，允许直接发送一个字符缓冲区，buff是要发送的数据，size是数据的大小，saddr是目标地址，len是地址的长度
            void SendInLoop(const char *buff, size_t size, struct sockaddr *saddr, socklen_t len);

            // 存储待发送的UDP数据包列表，使用std::list允许动态添加和删除数据包
            std::list<UdpBufferNodePtr> buffer_list_; 

            // 标记连接是否关闭，默认值为false，表示连接是打开的
            bool closed_{false};  

            // 最大空闲时间，单位：秒
            int32_t max_idle_time_{30};

            // 超时条目的弱指针，使用弱指针可以避免循环引用，允许在不拥有对象所有权的情况下引用它
            std::weak_ptr<UdpTimeoutEntry> timeout_entry_;

            // 消息缓冲区大小，默认值为65535字节，通常是UDP数据包的最大大小
            int32_t message_buffer_size_{65535}; 

            // 存储消息数据
            MsgBuffer message_buffer_; 

            // 消息回调，用于处理接收到的消息
            UdpSocketMessageCallback message_cb_;  

            // 写入完成回调，用于处理写入完成的事件
            UdpSocketWriteCompleteCallback write_complete_cb_; 

            // 关闭连接回调，用于处理连接关闭的事件
            UdpSocketCloseConnectionCallback close_cb_;
        };

        // 定义UdpTimeoutEntry结构体，用于管理与 UDP 套接字连接的超时处理
        // 确保在连接超时时能够正确处理相关逻辑，同时避免内存泄漏
        struct UdpTimeoutEntry
        {
            // 构造函数，接受一个UdpSocketPtr类型的参数c，表示一个指向UdpSocket的智能指针
            UdpTimeoutEntry(const UdpSocketPtr &c)
                : conn(c)       // 初始化连接
            {

            }

            ~UdpTimeoutEntry()
            {
                // 从弱指针conn获取一个强指针，如果conn指向的对象仍然存在，则c将是一个有效的强指针
                auto c = conn.lock();

                // 检查c是否有效，如果c是一个有效的强指针，表示对象仍然存在
                if (c)
                {
                    // 如果有效，处理超时逻辑
                    c->OnTimeOut();
                }
            }
            
            // 弱指针，指向UdpSocket对象，使用弱指针可以避免循环引用，允许在不拥有对象所有权的情况下引用它
            std::weak_ptr<UdpSocket> conn;
        }; 

    }
}