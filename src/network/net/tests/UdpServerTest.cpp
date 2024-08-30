#include <iostream>
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/UdpServer.h"

using namespace lss::network;

// 创建一个事件循环线程对象
EventLoopThread eventloop_thread;

// 创建一个标准线程对象
std::thread th;

int main(int argc, const char **agrv)
{
    // 启动事件循环线程
    eventloop_thread.Run();
    
    // 获取事件循环对象指针
    EventLoop *loop = eventloop_thread.Loop();

    // 如果事件循环对象有效
    if (loop)
    {
        // 初始化服务器的地址和端口
        InetAddress listen("172.22.88.236:34444");
        // 创建一个 UdpServer 对象，并使用智能指针管理它
        std::shared_ptr<UdpServer> server = std::make_shared<UdpServer>(loop, listen);

        // 设置接收消息的回调函数
        server->SetRecvMsgCallback([&server](const InetAddress &addr, MsgBuffer &buff){
            // 打印接收到的消息和发送方地址
            std::cout << "host: " << addr.ToIpPort() << " msg: " << buff.Peek() << std::endl;
            // 定义一个 sockaddr_in6 结构体用于存储地址
            struct sockaddr_in6 sock_addr;
            // 从 InetAddress 获取原始地址信息并存储到 sock_addr 中
            addr.GetSockAddr((struct sockaddr *)&sock_addr);
            // 使用服务器对象发送接收到的数据
            server->Send(buff.Peek(), buff.ReadableBytes(), (struct sockaddr *)&sock_addr, sizeof(sock_addr));
            // 清空缓冲区
            buff.RetrieveAll();
        });

        // 设置连接关闭时的回调函数
        server->SetCloseCallback([](const UdpSocketPtr &con){
            if( con)
            {
                // 打印连接关闭信息
                std::cout << "host: " << con->PeerAddr().ToIpPort() << " closed." << std::endl;
            }
        });

        // 设置写操作完成时的回调函数
        server->SetWriteCompleteCallback([](const UdpSocketPtr &con){
            if (con)
            {
                // 打印写完成的信息
                std::cout << "host: " << con->PeerAddr().ToIpPort() << " write complete. " << std::endl;
            }
        });

        // 启动服务器
        server->Start();

        // 主线程进入一个无限循环
        while (1)
        {
            // 线程休眠，定时休眠1秒钟
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}
