#include "EventLoopThread.h"

using namespace lss::network;

// 默认构造函数，使用成员初始化列表来初始化 thread_ 成员变量
// 在初始化 thread_ 时，使用 lambda 表达式作为参数，该 lambda 表达式会调用 StartEventLoop 函数，在构造函数结束时被调用
EventLoopThread::EventLoopThread()
: thread_([this](){StartEventLoop();})
{

}

// 析构函数
EventLoopThread::~EventLoopThread()
{
    // 调用 Run 函数，使 thread_ 等待事件循环结束
    Run();

    // 检查 loop_ 是否为空，如果为空，则说明 thread_ 已经结束，否则说明 thread_ 正在等待事件循环结束
    if (loop_)
    {
        // 调用 Quit 函数，使事件循环退出
        loop_->Quit();
    }

    // 检查 thread_ 是否可加入（joinable），如果是，调用 join 函数等待线程结束
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void EventLoopThread::Run()
{
    // 调用 std::call_once 函数，确保只调用一次
    std::call_once(once_, [this](){
        {
            // 调用 std::unique_lock<std::mutex> 构造函数，创建一个互斥锁，保护共享资源的访问
            std::lock_guard<std::mutex> lk(lock_);
            running_ = true;
            // 调用 std::condition_variable::notify_one 函数，通知其他等待线程
            condition_.notify_one();
        }
        // 获取一个与 promise_loop_ 关联的 std::future 对象
        auto f = promise_loop_.get_future();
        // 等待 std::future 对象的结果，将该结果传给 promise_loop_ 
        f.get();
    });
}

// 返回当前事件循环的指针 loop_
EventLoop *EventLoopThread::Loop() const
{
    return loop_;
}

// 启动事件循环
void EventLoopThread::StartEventLoop()
{
    // 创建对象
    EventLoop loop;
    // 调用 std::unique_lock<std::mutex> 构造函数，创建一个互斥锁，保护共享资源的访问
    std::unique_lock<std::mutex> lk(lock_);
    // 调用 std::condition_variable::wait 函数，等待其他线程调用 Run 函数
    // 等待事件循环线程启动，一旦 running_ 为 true ，将 loop_ 设置为指向 loop 对象的指针
    condition_.wait(lk, [this](){return running_;});
    loop_ = &loop;
    // 设置 promise_loop_ 的返回值为 1 
    promise_loop_.set_value(1);
    // 开始事件循环
    loop.Loop();
    // 事件循环结束，设置为 nullptr 
    loop_ = nullptr;
}
