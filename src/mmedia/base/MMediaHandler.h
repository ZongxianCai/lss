#pragma once
#include <memory>
#include "base/NonCopyable.h"
#include "network/net/TcpConnection.h"
#include "Packet.h"

namespace lss
{
    namespace mm
    {
        using namespace network;

        // 定义一个名为 MMediaHandler 的类，该类继承自 base::NonCopyable 类
        class MMediaHandler : public base::NonCopyable
        {
        public:
            // 纯虚函数：当有新连接时被调用，参数为连接的智能指针
            virtual void OnNewConnection(const TcpConnectionPtr &conn) = 0;

            // 纯虚函数：当连接销毁时被调用，参数为连接的智能指针
            virtual void OnConnectionDestroy(const TcpConnectionPtr &conn) = 0;

            // 纯虚函数：当收到数据时被调用，参数为连接的智能指针和接收的数据包智能指针
            virtual void OnRecv(const TcpConnectionPtr &conn ,const PacketPtr &data) = 0;

            // 纯虚函数：当收到数据时被调用，参数为连接的智能指针和数据包右值引用，处理数据的所有权转移
            virtual void OnRecv(const TcpConnectionPtr &conn ,PacketPtr &&data) = 0;

            // 纯虚函数：当连接处于活跃状态时被调用，参数为连接的智能指针
            virtual void OnActive(const ConnectionPtr &conn) = 0;
        };     
    }
}