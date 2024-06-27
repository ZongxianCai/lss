#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace lss
{
    namespace base
    {
        // 前置声明
        class Task;
        using TaskPtr = std::shared_ptr<Task>;
        using TaskCallback = std::function<void (const TaskPtr &)>;

        // 将类本身作为一个智能指针
        class Task : public std::enable_shared_from_this<Task>
        {
        public:
            // 回调构造函数
            Task(const TaskCallback &cb, int64_t intervel);

            // 右值回调构造函数
            Task(const TaskCallback &&cb, int64_t intervel);

            // 执行任务的方法
            void Run();

            // 重启任务的方法
            void Restart();

            // 获取当前的时间点
            int64_t When() const
            {
                return when_;
            }
        
        private:
            // 时间间隔
            int64_t interval_{0};

            // 执行任务的时间点（当前时间 + 时间间隔得到）
            int64_t when_{0};
            
            // 回调函数
            TaskCallback cb_;
        };
    }
}