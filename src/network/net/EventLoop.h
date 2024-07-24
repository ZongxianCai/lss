#pragma once
#include <vector>
#include <sys/epoll.h>
#include <memory>
#include <unordered_map>
#include "Event.h"

namespace lss
{
    namespace network
    {
        // 定义新的类型名称 EventPtr，指向 Event 对象的共享指针，使用 std::shared_ptr 处理引用计数，管理 Event 实例的生命周期    
        using EventPtr = std::shared_ptr<Event>;

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

            // // 声明一个使能函数 EnableReading ，用于启用或禁用读取事件，返回布尔值表示操作是否成功
            bool EnableEventReading(const EventPtr &event, bool enable);

        private:
            // 指示事件循环是否正在运行
            bool looping_{false};
            
            // 存储 epoll 文件描述符，初始值为 -1 ，表示未初始化
            int epoll_fd_{-1};

            // 存储 epoll 事件的集合
            std::vector<struct epoll_event> epoll_events_;

            // 存储已注册的事件，使用事件的标识符作为键，事件指针作为值
            std::unordered_map<int, EventPtr> events_;
        };
    }
}