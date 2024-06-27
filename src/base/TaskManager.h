#pragma once
#include "Task.h"
#include "Singleton.h"

namespace lss
{
    namespace base
    {
        class TaskManager : public NonCopyable
        {
        public:
            // 使用默认构造函数
            TaskManager() = default;

            // 使用默认析构函数
            ~TaskManager() = default;
        };
    }
}