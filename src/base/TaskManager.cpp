#include "TaskManager.h"
#include "TTime.h"

using namespace lss::base;

// 存放所有的定时任务
void TaskManager::OnWork()
{
    // RAII（资源获取即初始化），lk对象在构造时自动获取 lock_ 互斥锁，并在析构时释放锁
    std::lock_guard<std::mutex> lk(lock_);
    // 获取当前时间，与任务时间点进行对比
    int64_t now = TTime::NowMS();

    // 遍历tasks_中的所有任务
    for (auto iter = tasks_.begin(); iter != tasks_.end();)
    {
        // 检查当前迭代的任务是否到达执行时间
        if ((*iter)->When() < now)
        {
            // 执行任务
            (*iter)->Run();

            // 再次检查任务的执行时间是否小于当前时间，以确保任务不会重复执行
            if ((*iter)->When() < now)
            {
                // 如果任务的执行时间小于当前时间，从 tasks_ 中删除这个任务，并继续下一个迭代
                iter = tasks_.erase(iter);
                continue;
            }
        }

        // 如果任务还没有到执行时间，移动迭代器到下一个任务
        iter++;
    }
}

// 添加定时任务
bool TaskManager::Add(TaskPtr &task)
{
    // 获取互斥锁，RAII
    std::lock_guard<std::mutex> lk(lock_);
    // 在tasks_中查找任务，如果任务已经存在，返回迭代器
    auto iter = tasks_.find(task);

    // 任务存在，返回false
    if (iter != tasks_.end())
    {
        return false;
    }

    // 如果任务不存在，使用 emplace 方法将任务添加到 tasks_ 中
    tasks_.emplace(task);

    // 任务添加成功
    return true; 
}

// 删除定时任务
bool TaskManager::Del(TaskPtr & task)
{
    // 获取互斥锁，RAII
    std::lock_guard<std::mutex> lk(lock_);
    // 删除指定任务
    tasks_.erase(task);

    // 任务删除成功
    return true;
}