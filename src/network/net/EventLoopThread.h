#pragma once
#include "base/NonCopyable.h"
#include "EventLoop.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

namespace lss
{
    namespace network
    {
        class EventLoopThread
        {
        public:
            // 默认构造函数
            EventLoopThread();

            // 析构函数
            ~EventLoopThread();

            // 启动事件循环线程
            void Run();

            // 返回当前事件循环的指针
            EventLoop *Loop() const;

        private:
            // 启动事件循环
            void StartEventLoop();

            // 指向 EventLoop 对象的指针，用于保存当前的事件循环
            EventLoop *loop_{nullptr};

            // 事件循环的线程
            std::thread thread_;

            // 互斥锁，保护共享资源的访问
            std::mutex lock_;

            // 条件变量，用于线程间的同步
            std::condition_variable condition_;

            // 线程是否正在运行的标志
            bool running_{false};

            std::once_flag once_;

            std::promise<int> promise_loop_;
        };
    }
}