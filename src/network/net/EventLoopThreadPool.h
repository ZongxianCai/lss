#pragma once
#include <atomic>
#include "base/NonCopyable.h"
#include "EventLoop.h"
#include "EventLoopThread.h"

namespace lss
{
    namespace network
    {
        // 定义一个类型别名 EventLoopThreadPtr，表示指向 EventLoopThread 的智能指针
        using EventLoopThreadPtr = std::shared_ptr<EventLoopThread>;

        // 事件循环线程池，不可复制
        class EventLoopThreadPool : public base::NonCopyable
        {
        public:
            // 构造函数声明。接收线程数量、起始线程索引和 CPU 数量
            EventLoopThreadPool(int thread_num, int start = 0, int cpus = 4);
            
            // 析构函数，清理资源
            ~EventLoopThreadPool();

            // 获取事件循环，返回一个指向 EventLoop 的指针
            std::vector<EventLoop *> GetLoops() const;

            // 获取下一个事件循环，返回一个指向 EventLoop 的指针
            EventLoop *GetNextLoop();

            // 返回线程池中事件循环的数量
            size_t Size();
            
            // 启动线程池
            void Start();

        private:
            // 存储 EventLoopThreadPtr 类型的线程指针
            std::vector<EventLoopThreadPtr> threads_;

            // 原子整数 loop_index_ ，初始化为 0 ，用于跟踪下一个事件循环的索引
            std::atomic_int32_t loop_index_{0};
        };
    }
}