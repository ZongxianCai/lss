#include <iostream>
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"

using namespace lss::network;

// 创建 EventLoopThread 对象
EventLoopThread event_loop_thread;
// 声明 std::thread 对象
std::thread th;

int main(int argc, const char *argv[]) 
{
    // 启动事件循环线程
    event_loop_thread.Run();
    // 获取事件循环对象
    EventLoop *loop = event_loop_thread.Loop();

    // 如果事件循环对象有效
    if (loop)
    {
        // 创建 InetAddress 对象，指定服务器地址和端口
        InetAddress server("172.22.88.236:34444");
        // 创建 Acceptor 对象，使用事件循环和服务器地址
        std::shared_ptr<Acceptor> acceptor = std::make_shared<Acceptor>(loop, server);

        // 设置接收连接的回调函数
        acceptor->SetAcceptCallback([](int fd, const InetAddress &addr) {
            // 输出连接的主机地址和端口
            std::cout << "host: " << addr.ToIpPort() << std::endl;
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