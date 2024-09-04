#include <iostream>
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/TcpClient.h"
#include "mmedia/rtmp/RtmpHandShake.h"

using namespace lss::network;
using namespace lss::mm;

// 声明一个EventLoopThread对象，用于管理事件循环的线程
EventLoopThread eventloop_thread;
// 声明一个标准线程对象th
std::thread th;

// 定义一个智能指针类型RtmpHandShakePtr，用于管理RtmpHandShake对象的共享所有权
using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;

// 定义两个字符串常量，http_request和http_response，分别表示一个简单的HTTP GET请求和HTTP响应
const char *http_request = "GET / HTTP/1.0\r\nHost: 192.168.56.168\r\nAccept: */*\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";
const char *http_response = "HTTP/1.0 200 OK\r\nServer: lss\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, const char **agrv)
{
    // 启动事件循环线程，开始处理事件循环
    eventloop_thread.Run();

    // 获取事件循环对象指针，用于后续操作
    EventLoop *loop = eventloop_thread.Loop();

    if (loop)
    {
        // 创建一个InetAddress对象，用于存储服务器地址信息，IP地址为172.22.88.236，端口号为1935（通常为RTMP协议端口）
        InetAddress server("192.168.56.168:1935");

        // 创建一个TcpClient智能指针对象，表示一个TCP客户端，并将其与事件循环和服务器地址绑定
        std::shared_ptr<TcpClient> client = std::make_shared<TcpClient>(loop, server);

        // 设置TCP客户端的接收消息回调函数
        client->SetRecvMsgCallback([](const TcpConnectionPtr &con, MsgBuffer &buff){
            // 当接收到消息时，从连接对象的上下文中获取RtmpHandShake对象，并调用其HandShake方法处理数据
            RtmpHandShakePtr shake = con->GetContext<RtmpHandShake>(kNormalContext);
            shake->HandShake(buff);
        });

        // 设置TCP客户端的关闭连接回调函数
        client->SetCloseCallback([](const TcpConnectionPtr &con){
            if (con)
            {
                // 当连接关闭时，输出连接的对端IP地址和端口信息
                std::cout << "host: " << con->PeerAddr().ToIpPort() << " closed." << std::endl;
            }
        });

        // 设置TCP客户端的写入完成回调函数
        client->SetWriteCompleteCallback([](const TcpConnectionPtr &con){
            if (con)
            {
                 // 当数据写入完成时，输出连接的对端IP地址和端口信息，并调用RtmpHandShake对象的WriteComplete方法
                std::cout << "host: " << con->PeerAddr().ToIpPort() << " write complete. " << std::endl;
                RtmpHandShakePtr shake = con->GetContext<RtmpHandShake>(kNormalContext);
                shake->WriteComplete();
            }
        });

        // 设置TCP客户端的连接回调函数
        client->SetConnectCallback([](const TcpConnectionPtr&con, bool connected){
            if (connected)
            {
                // 当连接成功时，创建RtmpHandShake对象，并将其与连接关联，随后调用Start方法开始握手过程
                RtmpHandShakePtr shake = std::make_shared<RtmpHandShake>(con, true);
                con->SetContext(kNormalContext, shake);
                shake->Start();
                
            }
        });

        // 发起连接请求
        client->Connect();

        // 主线程进入一个无限循环，每次休眠1秒，保持程序运行
        while (1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}
