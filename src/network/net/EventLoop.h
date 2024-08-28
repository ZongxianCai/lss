#pragma once
#include <vector>
#include <sys/epoll.h>
#include <memory>
#include <unordered_map>
#include <functional>
#include <queue>
#include <mutex>
#include "Event.h"
#include "PipeEvent.h"
#include "TimingWheel.h"

namespace lss
{
    namespace network
    {
        // 定义新的类型名称 EventPtr，指向 Event 对象的共享指针，使用 std::shared_ptr 处理引用计数，管理 Event 实例的生命周期    
        using EventPtr = std::shared_ptr<Event>;

        // 定义一个可以接收无参数并且没有返回值的函数类型，使用 Func 来声明变量、参数或返回类型，以表示接受无参数并且没有返回值的函数
        using Func = std::function<void()>;

        class EventLoop
        {
        public:
            // 默认构造函数，用于初始化EventLoop类的实例
            EventLoop();
            
            // 析构函数，用于在对象销毁时释放资源
            ~EventLoop();

            // 启动事件循环
            void Loop();

            // 退出事件循环
            void Quit();

            // 添加一个事件到事件循环中进行处理
            void AddEvent(const EventPtr &event);

            // 从事件循环中删除一个事件
            void DelEvent(const EventPtr &event);

            // 声明一个使能函数 EnableWriting ，用于启用或禁用写入事件，返回布尔值表示操作是否成功
            bool EnableEventWriting(const EventPtr &event, bool enable);

            // 声明一个使能函数 EnableReading ，用于启用或禁用读取事件，返回布尔值表示操作是否成功
            bool EnableEventReading(const EventPtr &event, bool enable);

            // 检查当前线程是否是事件循环的所属线程
            void AssertInLoopThread();

            // 检查当前线程是否是在事件循环中的线程，const 修饰符表示该方法不会修改类的任何成员变量
            bool IsInLoopThread() const;

            // 接收一个对函数（或可调用对象）的常量引用，并安排在事件循环中执行，使用 const 表示传递的函数不会被修改
            void RunInLoop(const Func &func);

            // 对上一个方法的重载，接收一个右值引用的函数，允许将函数移动到事件循环中，对于临时对象来说可能更高效
            void RunInLoop(Func &&func);

            // 插入一个时间轮定时事件
            void InsertEntry(uint32_t delay, EntryPtr entryPtr);

            // 延迟 delay 后执行回调函数 cb
            void RunAfter(double delay, const Func &cb);

            // 延迟 delay 后执行右值引用的回调函数 cb
            void RunAfter(double delay, Func &&cb);

            // 每隔 interval 执行一次回调函数 cb
            void RunEvery(double interval, const Func &cb);

            // 每隔 interval 执行一次右值引用的回调函数 cb
            void RunEvery(double interval, Func &&cb);

        private:
            // 执行所有已安排在事件循环中运行的函数，在事件循环的处理阶段被调用
            void RunFunctions();

            // 唤醒事件循环并开始处理事件或已安排的函数，可以用于中断阻塞等待状态
            void WakeUp();

            // 指示事件循环是否正在运行
            bool looping_{false};
            
            // 存储 epoll 文件描述符，初始值为 -1 ，表示未初始化
            int epoll_fd_{-1};

            // 存储 epoll 事件的集合
            std::vector<struct epoll_event> epoll_events_;

            // 存储已注册的事件，使用事件的标识符作为键，事件指针作为值
            std::unordered_map<int, EventPtr> events_;

            // 声明一个队列，用于存储需要在事件循环中执行的函数对象
            std::queue<Func> functions_;

            // 声明一个互斥锁，保护共享资源的并发访问，在多线程环境中，lock_ 将用于对 events_ 和 functions_ 的访问进行同步
            std::mutex lock_;

            // 声明 pipe_event_ ，指向 PipeEvent 类型的智能指针，用于在事件循环中处理管道事件
            PipeEventPtr pipe_event_;

            // 声明一个时间轮定时器，用于在事件循环中处理定时事件
            TimingWheel wheel_;
        };
    }
}
