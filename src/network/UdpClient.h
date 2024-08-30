#pragma once
#include <functional>
#include "network/net/UdpSocket.h"

namespace lss
{
    namespace network
    {
        // 定义一个类型别名 ConnectedCallback，表示一个函数对象，该函数对象接受一个 UdpSocketPtr 引用和一个布尔值，并且不返回值
        using ConnectedCallback = std::function<void (const UdpSocketPtr &, bool)>;

        // 定义一个 UdpClient 类，继承自 UdpSocket 类
        class UdpClient:public UdpSocket
        {
        public:
            // 构造函数，接受一个 EventLoop 指针和一个服务器地址对象
            UdpClient(EventLoop *loop, const InetAddress &server);

            // 启动连接到服务器的操作
            void Connect();

            // 设置连接成功后的回调函数，接受一个常量引用作为参数
            void SetConnectedCallback(const ConnectedCallback &cb);

            // 设置连接成功后的回调函数，接受一个右值引用作为参数
            void SetConnectedCallback(ConnectedCallback &&cb);

            // 在事件循环中执行连接操作
            void ConnectInLoop();

            // 发送数据，接受一个 BufferNodePtr 指针列表作为参数
            void Send(std::list<BufferNodePtr> &list);

            // 发送数据，接受一个字符数组和大小作为参数
            void Send(const char *buff, size_t size);

            // 重写基类的 OnClose 方法，处理关闭连接的操作
            void OnClose() override;

            // 析构函数，销毁 UdpClient 对象
            virtual ~UdpClient();

        private:
            // 表示是否连接成功的标志位，初始值为 false
            bool connected_{false};

            // 服务器的地址
            InetAddress server_addr_;

            // 连接成功后的回调函数
            ConnectedCallback connected_cb_;

            // 用于存储 IPv6 地址信息的结构体
            struct sockaddr_in6 sock_addr_;

            // 存储地址长度的变量，初始化为 sockaddr_in6 结构体的大小
            socklen_t sock_len_{sizeof(struct sockaddr_in6)};
        };
    }
}