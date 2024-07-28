#include <iostream>
#include <thread>
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/net/EventLoopThreadPool.h"
#include "network/net/PipeEvent.h"
#include "base/TTime.h"

using namespace lss::network;

EventLoopThread event_loop_thread;

std::thread th;

void TestEventLoopThread()
{
    // 启动事件循环线程
    event_loop_thread.Run();

    // 获取事件循环对象的指针
    EventLoop *loop = event_loop_thread.Loop();

    // 如果 loop 不为空
    if (loop)
    {
        // 输出事件循环对象的地址到标准输出流
        std::cout << "loop: " << loop << std::endl;
        // 创建智能指针对象 pipe ，通过调用事件循环对象的 AddEvent 函数将其添加到事件循环中
        PipeEventPtr pipe = std::make_shared<PipeEvent>(loop);
        loop->AddEvent(pipe);
        int64_t test = 12345;
        // 将 12345 写入到管道中
        pipe->Write((const char*)&test, sizeof(test));

        // 创建一个新线程，使用 lambda 表达式作为线程的入口函数
        th = std::thread([&pipe](){
            // 不断获取当前时间戳，并将其写入管道中
            while (1)
            {
                int64_t now = lss::base::TTime::NowMS();
                pipe->Write((const char*)&now, sizeof(test));
                // 线程休眠 1 s
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        while (1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void TestEventLoopThreadPool()
{
    EventLoopThreadPool pool(2, 0, 2);
    
    pool.Start();

    std::vector<EventLoop *> list = pool.GetLoops();
    for (auto &e : list)
    {
        std::cout << "loop : " << e << std::endl;
    }

    EventLoop *loop = pool.GetNextLoop();
    std::cout << "loop : "  << loop << std::endl;

    loop = pool.GetNextLoop();
    std::cout << "loop : "  << loop << std::endl;
}

int main(int argc, const char **argv)
{
    // TestEventLoopThread();
    TestEventLoopThreadPool();

    return 0;
}