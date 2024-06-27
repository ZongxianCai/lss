#include "TaskManager.h"
#include "TTime.h"

using namespace lss::base;

// 存放所有的定时任务
void TaskManager::OnWork()
{
    // 获取锁，RAII
    std::lock_guard<std::mutex> lk(lock_);
    // 获取当前时间，与任务时间点进行对比
    int64_t now = TTime::NowMS();

    for (auto iter = tasks_.begin(); iter != tasks_.end();)
    {
        if ((*iter)->When() < now)
        {
            (*iter)->Run();

            if ((*iter)->When() < now)
            {
                iter = tasks_.erase(iter);
                continue;
            }
        }
        iter++;
    }
}

// 添加定时任务
bool TaskManager::Add(TaskPtr &task)
{
    // 获取锁，RAII
    std::lock_guard<std::mutex> lk(lock_);
    auto iter = tasks_.find(task);

    // 找到了，返回false
    if (iter != tasks_.end())
    {
        return false;
    }

    // 未找到，进行插入
    tasks_.emplace(task);

    return true; 
}

// 删除定时任务
bool TaskManager::Del(TaskPtr & task)
{
    // 获取锁，RAII
    std::lock_guard<std::mutex> lk(lock_);
    // 删除
    tasks_.erase(task);

    return true;
}