#pragma once
#include <functional>
#include "network/base/InetAddress.h"
#include "network/base/SocketOpt.h"
#include "network/net/Event.h"
#include "network/net/EventLoop.h"

namespace lss
{
    namespace network
    {
        // 定义一个类型别名 AcceptCallback ，表示一个接收回调函数，接收一个套接字和一个 InetAddress 对象
        using AcceptCallback = std::function<void(int sock, const InetAddress &addr)>;

        class Acceptor : public Event
        {
        public:
            // 构造函数，初始化 Acceptor 对象，接受一个事件循环和一个地址
            Acceptor(EventLoop *loop, const InetAddress &addr);

            // 设置接受连接的回调函数，使用常量引用以避免不必要的拷贝
            void SetAcceptCallback(const AcceptCallback &cb);

            //  设置接受连接的回调函数，接受右值引用，允许移动语义
            void SetAcceptCallback(AcceptCallback &&cb);

            // 启动接收器，开始监听连接请求
            void Start();

            // 停止接收器，停止监听连接请求
            void Stop();

            //  处理读取事件，重写基类的 OnRead 方法
            void OnRead() override;

            // 处理错误事件，重写基类的 OnError 方法
            void OnError(const std::string &msg) override;

            //  处理关闭事件，重写基类的 OnClose 方法
            void OnClose() override;

            // 析构函数，用于释放资源
            ~Acceptor();

        private:
            // 开启接收套接字
            void Open();

            // 存储InetAddress对象
            InetAddress addr_;

            // 存储接收回调函数
            AcceptCallback accept_cb_;

            // 指向 SocketOpt 对象的指针，初始化为 nullptr
            SocketOpt *socket_opt_{nullptr};
        };
    }
}