#include <iostream>
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/UdpClient.h"

using namespace lss::network;

// 创建一个事件循环线程对象
EventLoopThread eventloop_thread;

// 创建一个线程对象
std::thread th;

int main(int argc, const char **agrv)
{
    // 启动事件循环线程
    eventloop_thread.Run();

    // 获取事件循环线程中的事件循环对象指针
    EventLoop *loop = eventloop_thread.Loop();

    // 判断事件循环是否成功启动
    if (loop)
    {
        // 创建服务器地址对象
        InetAddress server("172.22.88.236:34444");

        // 创建一个 UdpClient 对象的智能指针
        std::shared_ptr<UdpClient> client = std::make_shared<UdpClient>(loop, server);

        // 设置接收消息的回调函数，处理接收到的消息
        client->SetRecvMsgCallback([](const InetAddress &addr, MsgBuffer &buff){
            // 打印接收到的消息
            std::cout << "host: " << addr.ToIpPort() << " msg: " << buff.Peek() << std::endl;
            // 清空消息缓冲区
            buff.RetrieveAll();
        });

        // 设置连接关闭的回调函数
        client->SetCloseCallback([](const UdpSocketPtr &con){
            if (con)
            {
                // 打印关闭连接的信息
                std::cout << "host: " << con->PeerAddr().ToIpPort() << " closed." << std::endl;
            }
        });

        // 设置写入完成的回调函数
        client->SetWriteCompleteCallback([](const UdpSocketPtr &con){
            if (con)
            {
                // 打印写入完成的信息
                std::cout << "host: " << con->PeerAddr().ToIpPort() << " write complete." << std::endl;
            }
        });

        // 设置连接成功的回调函数
        client->SetConnectedCallback([&client](const UdpSocketPtr &con, bool connected){
            if (connected)
            {
                // 连接成功后，发送消息 "11111"
                client->Send("11111", strlen("11111"));
            }
        });

        // 启动连接操作
        client->Connect();

        // 主线程进入无限循环，每隔一秒休眠一次
        while (1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    return 0;
}