#include <unistd.h>
#include "Event.h"
#include "EventLoop.h"

using namespace lss::network;

// 定义 Event 类的构造函数，接收一个指向 EventLoop 的指针
Event::Event(EventLoop *loop)
:loop_(loop)
{
    // 主体为空，不执行任何操作
}

// 定义 Event 类的构造函数，接收一个指向 EventLoop 的指针和一个文件描述符 fd 
Event::Event(EventLoop *loop, int fd)
:loop_(loop)        // 初始化成员变量 loop_，将传入的 loop 指针赋值给它
, fd_(fd)           // 初始化成员变量 fd_，将传入的 fd 赋值给它
{
    // 主体为空，完成初始化后不执行任何操作
}

// 定义 Event 类的析构函数
Event::~Event()
{
    // 文件描述符大于 0 ，关闭并恢复初始化值
    if (fd_ > 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
}

// 定义 Event 类的成员函数 EnableWriting，接受一个布尔参数 enable
bool Event::EnableWriting(bool enable)
{
    // 调用 loop_ 的 EnableEventWriting 方法，传入当前对象的共享指针和 enable 参数，并返回其结果
    return loop_->EnableEventWriting(shared_from_this(), enable);
}

// 定义 Event 类的成员函数 EnableReading，接受一个布尔参数 enable
bool Event::EnableReading(bool enable)
{
    // 调用 loop_ 的 EnableEventReading 方法，传入当前对象的共享指针和 enable 参数，并返回其结果
    return loop_->EnableEventReading(shared_from_this(), enable);
}

// 定义 Event 类的成员函数 Fd，返回文件描述符
int Event::Fd() const
{
    return fd_;
}