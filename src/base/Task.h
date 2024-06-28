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
        // 定义 TaskPtr 类型别名，是 Task 类的 std::shared_ptr，用于自动管理 Task 对象的生命周期
        using TaskPtr = std::shared_ptr<Task>;
        // 定义 TaskCallback 类型别名，是 std::function 的特化版本，表示接受 const TaskPtr & 作为参数并返回 void 的函数或函数对象。
        using TaskCallback = std::function<void (const TaskPtr &)>;

        // 定义 Task 类，继承自 std::enable_shared_from_this，允许 Task 对象通过 shared_from_this 方法获取自身的 shared_ptr
        class Task : public std::enable_shared_from_this<Task>
        {
        public:
            // 构造函数，接受 TaskCallback 类型的引用和 int64_t 类型的间隔，用于初始化任务的回调和执行间隔
            Task(const TaskCallback &cb, int64_t interval);

            // 接受 TaskCallback 类型的右值引用，允许使用临时的或右值引用的回调函数
            Task(const TaskCallback &&cb, int64_t interval);

            // 执行任务的回调函数
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