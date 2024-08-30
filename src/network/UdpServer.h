#pragma once
#include "network/net/UdpSocket.h"
#include "network/net/EventLoop.h"
#include "network/base/InetAddress.h"
#include "network/base/SocketOpt.h"

namespace lss
{
    namespace network
    {
        // 定义 UdpServer 类，继承自 UdpSocket 类
        class UdpServer : public UdpSocket
        {
        public:
            // 构造函数，接受一个事件循环对象指针和服务器地址对象作为参数
            UdpServer(EventLoop *loop, const InetAddress &server);

            // 启动服务器的方法
            void Start();

            // 停止服务器的方法
            void Stop();

            // 析构函数，对象销毁时释放资源
            virtual ~UdpServer();
            
        private:
            // 打开服务器的方法，用于初始化或绑定套接字
            void Open();

            // 保存服务器地址
            InetAddress server_;
        };
    }
}