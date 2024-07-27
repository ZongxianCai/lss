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
            EventLoopThread();

            ~EventLoopThread();

            void Run();

            EventLoop *Loop() const;

        private:
            void StartEventLoop();

            EventLoop *loop_{nullptr};

            std::thread thread_;

            std::mutex lock_;

            std::condition_variable condition_;

            bool running_{false};

            std::once_flag once_;

            std::promise<int> promise_loop_;
        };
    }
}