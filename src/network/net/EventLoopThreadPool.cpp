#include <pthread.h>
#include "EventLoopThreadPool.h"

using namespace lss::network;

// 定义一个匿名命名空间，限制其中的函数只在当前文件中可见
namespace
{
    // 绑定 CPU ，接收一个线程引用和一个 CPU 核心编号
    void bind_cpu(std::thread &t, int n)
    {
        // 定义一个 cpu_set_t 类型的变量，用于设置 CPU 亲和性
        cpu_set_t cpu;
        // 清空 cpu_set_t 变量，初始化为零
        CPU_ZERO(&cpu);
        // 将指定的 CPU 核心编号 n 添加到 cpu_set_t 变量中
        CPU_SET(n, &cpu);

        // 设置线程 t 的 CPU 亲和性，使其只在指定的核心上运行
        pthread_setaffinity_np(t.native_handle(), sizeof(cpu), &cpu);
    }
}

EventLoopThreadPool::EventLoopThreadPool(int thread_num, int start, int cpus)
{
    // 检查线程数量
    if (thread_num <= 0)
    {
        thread_num = 1;
    }

    // 循环创建指定数量的线程
    for (int i = 0; i < thread_num; i++)
    {
        // 创建一个 EventLoopThread 的共享指针并添加到线程容器中
        threads_.emplace_back(std::make_shared<EventLoopThread>());

        // 如果指定了 CPU 核心数量
        if (cpus > 0)
        {
            // 计算当前线程应该绑定的 CPU 核心编号
            int n = (start + i) % cpus;
            // 调用 bind_cpu 函数，将当前线程绑定到指定的 CPU 核心
            bind_cpu(threads_.back()->Thread(), n);
        }
    }
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // EventLoopThreadPool 类的成员变量是智能指针（std::shared_ptr），会自动管理内存
    // 当 EventLoopThreadPool 对象被销毁时，智能指针会自动释放其管理的资源，不需要在析构函数中手动释放资源
}

// 获取所有 EventLoop 的指针，返回类型为 std::vector<EventLoop *>
std::vector<EventLoop *> EventLoopThreadPool::GetLoops() const
{
    // 定义一个空的 EventLoop 指针向量，用于存储结果
    std::vector<EventLoop *> result;

    // 遍历线程容器 threads_
    for (auto &t : threads_)
    {
        // 将每个线程的 EventLoop 指针添加到结果向量中
        result.push_back(t->Loop());
    }

    // 返回包含所有 EventLoop 指针的向量
    return result;
}

// 获取下一个 EventLoop 的指针
EventLoop *EventLoopThreadPool::GetNextLoop()
{
    // 将当前索引赋值给 index
    int index = loop_index_;
    // 增加索引，以便下次调用时获取下一个 EventLoop
    loop_index_++;

    // 返回当前索引对应的 EventLoop 指针，使用取模运算确保索引在有效范围内
    return threads_[index % threads_.size()]->Loop();
}

// 获取线程池中线程的数量
size_t EventLoopThreadPool::Size()
{
    // 返回线程容器的大小
    return threads_.size();
}

// 启动线程池中的所有线程
void EventLoopThreadPool::Start()
{
    for (auto &t : threads_)
    {
        // 调用每个线程的 Run 方法，启动线程
        t->Run();
    }
}