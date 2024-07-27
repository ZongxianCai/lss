#pragma once
#include <memory>
#include "Event.h"

namespace lss
{
    namespace network
    {
        class PipeEvent : public Event
        {
        public:
            // 构造函数，接受一个 EventLoop 指针作为参数
            PipeEvent(EventLoop *loop);
            
            // 析构函数
            ~PipeEvent();

            // 重写基类的虚函数，提供自定义的实现，以满足派生类特定的需求
            void OnRead() override;

            void OnClose() override;

            void OnError(const std::string &msg) override;

            // 写入数据
            void Write(const char *data, size_t len);

        private:
            int write_fd_{-1};
        };

        // 自定义的类型别名 PipeEventPtr ，用于表示指向 PipeEvent 对象的智能指针，可以在对象不再需要时自动释放内存，避免内存泄漏
        using PipeEventPtr = std::shared_ptr<PipeEvent>;
    }
}