#include <iostream>
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/TcpClient.h"

using namespace lss::network;

// 创建事件循环线程对象
EventLoopThread eventloop_thread;
// 创建一个线程对象
std::thread th;

// 定义HTTP请求字符串
const char *http_request = "GET / HTTP/1.0\r\nHost: 172.22.88.236\r\nAccept: */*\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";
// 定义HTTP响应字符串
const char *http_response = "HTTP/1.0 200 OK\r\nServer: lss\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, const char ** agrv)
{
    // 启动事件循环线程
    eventloop_thread.Run();
    // 获取事件循环指针
    EventLoop *loop = eventloop_thread.Loop();

    // 如果事件循环有效
    if (loop)
    {
        // 创建一个InetAddress对象，指定监听地址和端口
        InetAddress server("172.22.88.236:34444");
        // 创建TcpClient对象
        std::shared_ptr<TcpClient> client = std::make_shared<TcpClient>(loop, server);

        // 设置消息回调函数
        client->SetRecvMsgCallback([](const TcpConnectionPtr &con, MsgBuffer &buff){
            // 输出接收到的消息
            std::cout << "host: " << con->PeerAddr().ToIpPort() << " msg: " << buff.Peek() << std::endl;
            // 清空消息缓冲区
            buff.RetrieveAll();
        });

        // 设置连接关闭回调函数
        client->SetCloseCallback([](const TcpConnectionPtr &con){
            // 如果连接关闭
            if (con)
            {
                // 输出连接关闭的信息
                std::cout << "host: " << con->PeerAddr().ToIpPort() << " closed." << std::endl;
            }
        });

        // 设置写完成回调函数
        client->SetWriteCompleteCallback([](const TcpConnectionPtr &con){
            // 如果写完成
            if (con)
            {
                // 输出写完成的信息
                // std::cout << "host:" << con->PeerAddr().ToIpPort() << " write complete." << std::endl;
            }
        });

        // 设置连接成功回调函数
        client->SetConnectCallback([](const TcpConnectionPtr &con, bool connected){
            // 如果连接成功
            if (connected)
            {
                // 发送数据
                con->Send(http_request, strlen(http_request));
            }
        });

        // 启动连接
        client->Connect();

        // 无限循环，保持程序运行
        while (1)
        {
            // 每秒休眠一次
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }   

    return 0;
}