#include "Task.h"
#include "TTime.h"

using namespace lss::base;

// 回调构造函数的实现
Task::Task(const TaskCallback &cb, int64_t interval)
: interval_(interval), when_(TTime::NowMS() + interval), cb_(cb)
{

}

// 右值回调构造函数的实现
Task::Task(const TaskCallback &&cb, int64_t interval)
: interval_(interval), when_(TTime::NowMS() + interval), cb_(std::move(cb))
{

}

// 执行任务方法的实现
void Task::Run()
{
    if (cb_)
    {
        cb_(shared_from_this());
    }
}

// 重启任务方法的实现
void Task::Restart()
{
    // 更新时间点
    when_ = interval_ + TTime::NowMS();
}