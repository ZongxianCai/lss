#include <iostream>
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "mmedia/rtmp/RtmpServer.h"
#include "mmedia/rtmp/RtmpHandShake.h"

using namespace lss::network;
using namespace lss::mm;

// 创建一个事件循环线程对象
EventLoopThread eventloop_thread;

// 创建一个标准线程对象
std::thread th;

// 定义 RtmpHandShake 的智能指针类型别名
using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;

// 定义一个简单的 HTTP 响应消息字符串
const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";

int main(int argc, const char **agrv)
{
    // 启动事件循环线程
    eventloop_thread.Run();

    // 获取事件循环指针
    EventLoop *loop = eventloop_thread.Loop();

    // 如果事件循环成功启动
    if (loop)
    {
        // 创建一个监听地址，指定 IP 和端口
        InetAddress listen("192.168.56.168:1935");

        // 创建一个 RTMP 服务器对象，传入事件循环和监听地址
        RtmpServer server(loop, listen);

        // 启动 RTMP 服务器
        server.Start();

        // 无限循环，保持主线程活动
        while (1)
        {
            // 每秒钟休眠一次
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}