#pragma once
#include <string>
#include <sys/epoll.h>
#include <memory>

namespace lss
{
    namespace network
    {
        // 前向声明 EventLoop 类，该类在其他地方定义，允许在 Event 类中使用指向 EventLoop 的指针
        // 声明一个前置类是一种向编译器声明的方式，诉编译器在当前文件中存在一个名为 EventLoop 的类
        // 这种声明只提供了类的名称，没有提供关于类的成员变量和成员函数的信息,在需要使用指针或引用指向 EventLoop 类的情况下很有用，而不需要完整的类定义
        // 这种前置声明有助于避免循环依赖，并且可以减少编译时间，因为不需要包含完整的类定义。
        class EventLoop;

        // 定义一个常量整数，用于表示一个事件的可读状态，通过按位或运算符 | 组合了三个事件标志（可读事件|优先可读事件|边缘触发模式）
        const int kEventRead = (EPOLLIN|EPOLLPRI|EPOLLET);

        // 定义一个常量整数，用于表示一个事件的可写状态，通过按位或运算符 | 组合了两个事件标志（可写事件|边缘触发模式）
        const int kEventWrite = (EPOLLOUT|EPOLLET);

        class Event : public std::enable_shared_from_this<Event>
        {
            // 声明友元类 EventLoop，允许在 Event 类中访问私有成员变量 loop_
            friend class EventLoop;
        public:
            // 默认构造函数，用于初始化 Event 类的实例
            Event();

            // 带参构造函数，接收一个指向 EventLoop 的指针和一个文件描述符 fd ，用于初始化 Event 类的实例
            Event(EventLoop *loop, int fd);

            // 析构函数，用于在对象销毁时释放资源
            ~Event();

            // 声明一个虚函数 OnRead ，用于处理可读事件，默认实现为空，允许子类重写该函数以实现特定行为
            virtual void OnRead() {};

            // 声明一个虚函数 OnWrite ，用于处理可写事件，默认实现为空，允许子类重写该函数以实现特定行为
            virtual void OnWrite() {};

            // 声明一个虚函数 OnClose ，用于处理关闭事件，默认实现为空，允许子类重写该函数以实现特定行为
            virtual void OnClose() {};

            // 声明一个虚函数 OnError ，用于处理错误事件，默认实现为空，允许子类重写该函数以实现特定行为
            virtual void OnError(const std::string &msg) {};

            // 声明一个使能函数 EnableWriting ，用于启用或禁用写入事件，返回布尔值表示操作是否成功
            bool EnableWriting(bool enable);

            // 声明一个使能函数 EnableReading ，用于启用或禁用读取事件，返回布尔值表示操作是否成功
            bool EnableReading(bool enable);

            // 声明一个公共成员函数 Fd ，返回当前事件的文件描述符，使用 const 修饰符表示该函数不会修改类的状态
            int Fd() const;

        private:
            // 声明一个指向 EventLoop 的私有成员变量 loop_，并初始化为 nullptr，表示未指向任何有效的 EventLoop 实例
            EventLoop *loop_{nullptr};

            // 声明一个整型的私有成员变量 fd，用于存储文件描述符，初始值为 -1，表示未初始化
            int fd_{-1};

            // 声明一个整型的私有成员变量 event，用于存储事件，初始值为 0
            int event_{0};
        };
    }
} 