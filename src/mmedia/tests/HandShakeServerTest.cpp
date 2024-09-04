#include <iostream>
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/TcpServer.h"
#include "mmedia/rtmp/RtmpHandShake.h"

using namespace lss::network;
using namespace lss::mm;

// 创建一个事件循环线程对象，用于处理异步事件循环
EventLoopThread eventloop_thread;

// 创建一个标准的C++线程对象
std::thread th;

// 定义一个智能指针类型RtmpHandShakePtr，指向RtmpHandShake类，用于管理握手对象的内存
using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;

// 定义一个常量字符指针，表示一个简单的HTTP响应消息
const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, const char **agrv)
{
    // 启动事件循环线程，进入事件循环
    eventloop_thread.Run();

    // 获取事件循环对象的指针
    EventLoop *loop = eventloop_thread.Loop();

    // 判断事件循环对象是否成功获取
    if (loop)
    {
        // 定义一个InetAddress对象，表示服务器监听的IP地址和端口号（RTMP协议默认使用1935端口）
        InetAddress listen("172.22.88.236:1935");

        // 创建一个TcpServer对象，绑定事件循环和监听地址，准备接受客户端连接
        TcpServer server(loop, listen);

        // 设置消息回调函数，当服务器接收到消息时触发
        server.SetMessageCallback([](const TcpConnectionPtr &con, MsgBuffer &buff){
            // 从连接上下文中获取RtmpHandShake对象，用于处理RTMP握手
            RtmpHandShakePtr shake = con->GetContext<RtmpHandShake>(kNormalContext);
            // 调用握手对象的HandShake函数，处理接收到的消息数据
            shake->HandShake(buff);
        });

        // 设置新连接回调函数，当有新客户端连接时触发
        server.SetNewConnectionCallback([&loop](const TcpConnectionPtr &con){
                // 为新连接创建一个RtmpHandShake对象，并用智能指针管理其生命周期
                RtmpHandShakePtr shake = std::make_shared<RtmpHandShake>(con, false);
                // 将创建的握手对象存储到连接的上下文中，以便后续使用
                con->SetContext(kNormalContext, shake);
                // 启动握手过程
                shake->Start();
                // 设置写入完成回调函数，当数据成功写入客户端后触发
                con->SetWriteCompleteCallback([&loop](const TcpConnectionPtr &con){
                // 打印消息，表明写入操作已完成，并输出客户端的IP和端口信息
                std::cout << "write complete host: " << con->PeerAddr().ToIpPort() << std::endl;
                // 从连接上下文中再次获取RtmpHandShake对象
                RtmpHandShakePtr shake = con->GetContext<RtmpHandShake>(kNormalContext);
                // 调用握手对象的WriteComplete函数，进一步处理写入完成后的逻辑
                shake->WriteComplete();
            });
        });

        // 启动服务器，开始监听和处理连接
        server.Start();

        while (1)
        {
            // 每隔1秒休眠一次，防止主线程退出
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}