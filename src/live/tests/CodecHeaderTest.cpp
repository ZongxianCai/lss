#include <iostream>
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/TcpClient.h"
#include "mmedia/rtmp/RtmpClient.h"
#include "live/CodecHeader.h"
#include "live/base/CodecUtils.h"

using namespace lss::network;
using namespace lss::mm;
using namespace lss::live;

// 创建一个事件循环线程
EventLoopThread eventloop_thread;

// 声明一个标准线程对象
std::thread th;

// 创建CodecHeader对象，用于处理音视频包的头信息
CodecHeader codec_header;

// 定义RtmpHandlerImpl类，继承自RtmpHandler，用于处理RTMP协议的事件
class RtmpHandlerImpl:public RtmpHandler
{
public:
    // 当有新的连接建立时的回调函数
    void OnNewConnection(const TcpConnectionPtr &conn) override
    {

    }

    // 当连接销毁时的回调函数
    void OnConnectionDestroy(const TcpConnectionPtr &conn) override
    {

    }

    // 当接收到数据时的回调函数，传入的是const引用的PacketPtr
    void OnRecv(const TcpConnectionPtr &conn ,const PacketPtr &data) override
    {
        //std::cout << "recv type: " << data->PacketType() << " size: " << data->PacketSize() << std::endl;
        // 检查接收到的数据是否是音视频的头信息，如果是，解析该包
        if (CodecUtils::IsCodecHeader(data))
        {
            // 调用CodecHeader对象解析头信息
            codec_header.ParseCodecHeader(data);
        }    
    }

    // 当接收到数据时的回调函数，传入的是右值引用的PacketPtr
    void OnRecv(const TcpConnectionPtr &conn, PacketPtr &&data) override
    {
        //std::cout << "recv type: " << data->PacketType() << " size: " << data->PacketSize() << std::endl;
        // 检查接收到的数据是否是音视频的头信息，如果是，解析该包
        if (CodecUtils::IsCodecHeader(data))
        {
            // 调用CodecHeader对象解析头信息
            codec_header.ParseCodecHeader(data);
        }
    }

    // 当连接激活时的回调函数
    void OnActive(const ConnectionPtr &conn) 
    {

    }

    // 当接收到播放请求时的回调函数
    bool OnPlay(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) {return false;}

    // 当接收到发布流的请求时的回调函数
    virtual bool OnPublish(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) {return false;}

    // 当接收到暂停请求时的回调函数
    virtual void OnPause(const TcpConnectionPtr &conn, bool pause){}

    // 当接收到跳转请求时的回调函数
    virtual void OnSeek(const TcpConnectionPtr &conn, double time){}
};

int main(int argc, const char **agrv)
{
    // 启动事件循环线程
    eventloop_thread.Run();

    // 获取事件循环对象
    EventLoop *loop = eventloop_thread.Loop();

    // 如果获取到了事件循环
    if (loop)
    {
        // 创建RtmpClient对象，传入事件循环和自定义的RtmpHandlerImpl处理器
        RtmpClient client(loop, new RtmpHandlerImpl());
        // 启动播放，连接到指定的RTMP流
        client.Play("rtmp://czx.com/live/test");
        
        while(1)
        {
            // 每秒钟暂停1秒
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }    
    return 0;
}