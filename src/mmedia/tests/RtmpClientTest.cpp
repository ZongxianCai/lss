#include <iostream>
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/TcpClient.h"
#include "mmedia/rtmp/RtmpClient.h"

using namespace lss::network;
using namespace lss::mm;

// 创建一个 EventLoopThread 对象，用于管理事件循环线程
EventLoopThread eventloop_thread;

// 声明一个标准线程对象
std::thread th;

// 继承自 RtmpHandler 类，用于处理 RTMP 相关事件
class RtmpHandlerImpl : public RtmpHandler
{
public:
    // 当有新连接建立时的回调函数
    void OnNewConnection(const TcpConnectionPtr &conn) override
    {

    }

    // 当连接销毁时的回调函数
    void OnConnectionDestroy(const TcpConnectionPtr &conn)override
    {

    }

    // 接收到数据时的回调函数，打印接收到的数据包类型和大小
    void OnRecv(const TcpConnectionPtr &conn, const PacketPtr &data)override
    {
        std::cout << "recv type: " << data->PacketType() << " size: " << data->PacketSize() << std::endl;
    }

    // 重载的接收数据回调函数，处理接收到的数据（右值版本）
    void OnRecv(const TcpConnectionPtr &conn, PacketPtr &&data) override
    {
        std::cout << "recv type: " << data->PacketType() << " size: " << data->PacketSize() << std::endl;
    }

    // 连接激活时的回调函数
    void OnActive(const ConnectionPtr &conn) 
    {

    }

    // 当播放请求到达时的回调函数，返回布尔值表示是否允许播放
    bool OnPlay(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) 
    {
        return false;
    }

    // 当发布请求到达时的回调函数，返回布尔值表示是否允许发布
    virtual bool OnPublish(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) 
    {
        return false;
    }

    // 当收到暂停请求时的回调函数
    virtual void OnPause(const TcpConnectionPtr &conn, bool pause)
    {

    }

    // 当收到跳转播放请求时的回调函数
    virtual void OnSeek(const TcpConnectionPtr &conn, double time)
    {
        
    }
};

int main(int argc, const char **agrv)
{
    // 启动事件循环线程，处理网络事件
    eventloop_thread.Run();

    // 获取事件循环对象
    EventLoop *loop = eventloop_thread.Loop();

    // 如果事件循环对象有效
    if (loop)
    {
        // 创建 RtmpClient 对象，并传入事件循环对象和自定义的 RTMP 处理器
        RtmpClient client(loop, new RtmpHandlerImpl());

        // 调用 Play 函数，开始播放指定的 RTMP 流媒体URL
        client.Play("rtmp://hx.com/live/test");

        // 无限循环，保持主线程不退出
        while (1)
        {
            // 每次循环中，主线程休眠1秒
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }    

    return 0;
}