#pragma once

#include <unordered_set>
#include <mutex>
#include "Task.h"
#include "Singleton.h"

namespace lss
{
    namespace base
    {
        // 定时任务管理器类
        class TaskManager : public NonCopyable
        {
        public:
            // 使用默认构造函数，不进行任何操作
            TaskManager() = default;

            // 使用默认析构函数，不进行任何操作
            ~TaskManager() = default;

            // 存放所有的定时任务
            void OnWork();

            // 添加定时任务
            bool Add(TaskPtr &task);

            // 删除定时任务
            bool Del(TaskPtr & task);

        private:
            // 存储和管理任务，存储Task类型的智能指针
            std::unordered_set<TaskPtr> tasks_;

            // 互斥锁，用于同步访问任务集合，保证多线程环境下的线程安全
            std::mutex lock_;
        };
    }
    
    #define taskManager lss::base::Singleton<lss::base::TaskManager>::Instance()
}