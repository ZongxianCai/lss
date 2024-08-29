#include <iostream>
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/net/TcpConnection.h"

using namespace lss::network;

// 创建 EventLoopThread 对象
EventLoopThread event_loop_thread;
// 声明 std::thread 对象
std::thread th;

// 定义 HTTP 响应
const char *http_response="HTTP/1.0 200 OK\r\nServer: lss\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, const char *argv[]) 
{
    // 启动事件循环线程
    event_loop_thread.Run();
    // 获取事件循环对象
    EventLoop *loop = event_loop_thread.Loop();

    // 如果事件循环对象有效
    if (loop)
    {
        // 声明一个 TcpConnectionPtr 列表
        std::vector<TcpConnectionPtr> list;
        // 创建 InetAddress 对象，指定服务器地址和端口
        InetAddress server("172.22.88.236:34444");
        // 创建 Acceptor 对象，使用事件循环和服务器地址
        std::shared_ptr<Acceptor> acceptor = std::make_shared<Acceptor>(loop, server);

        // 设置接收连接的回调函数
        acceptor->SetAcceptCallback([&loop, &server, &list](int fd, const InetAddress &addr) {
            // 输出连接的主机地址和端口
            std::cout << "host: " << addr.ToIpPort() << std::endl;
            // 创建一个 TcpConnection 的智能指针
            TcpConnectionPtr connection = std::make_shared<TcpConnection>(loop, fd, server, addr);

            // 设置接收消息的回调函数
            connection->SetRecvMsgCallback([](const TcpConnectionPtr &con, MsgBuffer &buff) {
                // 输出接收到的消息
                std::cout << "recv msg: " << buff.Peek() << std::endl;
                // 清空消息缓冲区
                buff.RetrieveAll();
                // 发送 HTTP 响应
                con->Send(http_response, strlen(http_response));
            });

            // 设置写完成的回调函数
            connection->SetWriteCompleteCallback([&loop](const TcpConnectionPtr &con) {
                // 输出写入完成的主机地址
                std::cout << "write complete host: " << con->PeerAddr().ToIpPort() << std::endl;
                // 从事件循环中删除该连接
                loop->DelEvent(con);
                // 强制关闭连接
                con->ForceClose();
            });

            // 测试超时功能是否实现，需要把 48 行发送 HTTP 响应代码注释掉
            // connection->EnableCheckIdleTimeout(3);

            // 将该连接添加到连接列表中
            list.push_back(connection);
            // 将该连接添加到事件循环中
            loop->AddEvent(connection);
        });

        // 启动 Acceptor，开始接受连接
        acceptor->Start();

        // 无限循环
        while (1)
        {
            // 每秒休眠一次
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return 0;
}