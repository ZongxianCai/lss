#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "EventLoop.h"
#include "network/base/Network.h"

using namespace lss::network; 

//  定义一个线程局部变量，用于存储当前线程的事件循环实例指针
static thread_local EventLoop *t_local_event_loop = nullptr;

// 构造函数，初始化事件循环
EventLoop::EventLoop()
: epoll_fd_(::epoll_create(1024))   // 创建一个 epoll 实例，返回的文件描述符存储在 epoll_fd_ 变量中
, epoll_events_(1024)               // 初始化 epoll_events_ ，大小不超过1024，则不会重新分配内存，减少频繁分配内存带来的性能下降问题
{
    // 检测是否已经存在事件循环实例，若存在，程序退出
    if (t_local_event_loop)
    {
        NETWORK_ERROR << "There already had an event loop ! ";
        exit(-1);
    }

    // 将当前实例指针赋值给线程局部变量
    t_local_event_loop = this;
}

// 析构函数，调用 Quit 函数，清理资源
EventLoop::~EventLoop()
{
    Quit();
}

// 事件循环主方法，使用 epoll 机制处理网络事件
void EventLoop::Loop()
{
    // 初始化循环状态
    looping_ = true;
    
    // 进入主循环
    while (looping_)
    {
        // 清空事件数组，准备接收新的事件
        memset(&epoll_events_[0], 0x00, sizeof(struct epoll_event)*epoll_events_.size());

        // 调用 epoll_wait 函数，等待事件发生，直到有事件到达时进行阻塞
        auto ret = ::epoll_wait(epoll_fd_, (struct epoll_event*)&epoll_events_[0], static_cast<int>(epoll_events_.size()), -1);

        // 如果事件数大于 0 ，则进行处理
        if (ret > 0)
        {
            // 遍历所有返回的事件
            for (int i = 0; i < ret; i++)
            {
                struct epoll_event &ev = epoll_events_[i];
                
                // 文件描述符无效，跳过该事件
                if (ev.data.fd <= 0)
                {
                    continue;
                }

                // 有效，查找对应的事件
                auto iter = events_.find(ev.data.fd);
                if (iter == events_.end())
                {
                    continue;
                }

                EventPtr &event = iter->second;

                // 如果发生错误，获取错误信息并调用 OnError 函数进行处理
                if (ev.events & EPOLLERR)
                {
                    int error = 0;
                    socklen_t len = sizeof(error);
                    getsockopt(event->Fd(), SOL_SOCKET, SO_ERROR, &error, &len);

                    event->OnError(strerror(error));
                }
                else if ((ev.events & EPOLLHUP) && !(ev.events & EPOLLIN))
                {
                    // 处理关闭事件，调用 OnClose 函数
                    event->OnClose();
                }
                else if (ev.events & (EPOLLIN | EPOLLPRI))
                {
                    // 处理读事件，调用 OnRead 函数
                    event->OnRead();
                }
                else if (ev.events & EPOLLOUT)
                {
                    // 处理写事件，调用 OnWrite 函数
                    event->OnWrite();
                }
            }

            // 如果返回的事件数量等于当前数组大小，扩大 epoll_events_ 数组的大小
            if (ret == epoll_events_.size())
            {
                epoll_events_.resize(epoll_events_.size() * 2);
            }
        }
        else if (ret == 0)
        {

        }
        else if (ret < 0)
        {
            NETWORK_DEBUG << "epoll_wait error, error : ", errno;
        }
    }
}

// 安全停止事件循环，使得 EventLoop::Loop() 函数能够结束其循环
void EventLoop::Quit()
{
    // 停止正在运行的事件循环
    looping_ = false;
}

// 添加一个事件到事件循环中进行处理，如果不存在，则将其添加到事件列表中，并通过 epoll_ctl 注册到 epoll 实例中以进行事件监听
void EventLoop::AddEvent(const EventPtr &event)
{
    // 使用事件的文件描述符（Fd()）在 events_ 容器中查找对应的事件，如果找到，iter 将指向该事件的迭代器
    auto iter = events_.find(event->Fd()); 
    if (iter != events_.end())
    {
        // 找到了，该事件已经存在，直接返回，不再添加
        return;
    }
    
    // 将事件的标志位与 kEventRead 进行按位或操作，表示该事件需要读取
    event->event_ |= kEventRead;
    // 将事件添加到 events_ 容器中，以文件描述符为键，事件为值
    events_[event->Fd()] = event;

    // 定义一个 epoll_event 结构体变量 ev，用于存储事件信息
    struct epoll_event ev;
    // 将 ev 结构体的所有字节初始化为 0
    memset(&ev, 0x00, sizeof(struct epoll_event));
    // 更新事件的标志位
    ev.events = event->event_;
    // 更新事件的文件描述符
    ev.data.fd = event->fd_;
    // 调用 epoll_ctl 函数，将事件添加到 epoll 实例中，使用 EPOLL_CTL_ADD 操作
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event->fd_, &ev);
}

// 从事件循环中查找并删除指定事件，同时确保在 epoll 实例中也将该事件移除
void EventLoop::DelEvent(const EventPtr &event)
{
    // 使用事件的文件描述符（Fd()）在 events_ 容器中查找对应的事件，如果找到，iter 将指向该事件的迭代器
    auto iter = events_.find(event->Fd());
    if (iter == events_.end())
    {
        return;
    }
    
    // 找到了事件，则从 events_ 容器中删除该事件
    events_.erase(iter);

    // 存储事件信息
    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.events = event->event_;
    ev.data.fd = event->fd_;
    //  调用 epoll_ctl 函数，使用 EPOLL_CTL_DEL 操作将事件从 epoll 实例中删除
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event->fd_, &ev);
}

// 根据传入的参数启用或禁用特定事件的写入功能，并更新 epoll 事件
bool EventLoop::EnableEventWriting(const EventPtr &event, bool enable)
{
    auto iter = events_.find(event->Fd());
    // 如果未找到对应的事件，输出错误信息并返回 false
    if (iter == events_.end())
    {
        NETWORK_ERROR << "Cannot find event fd: " << event->Fd();
        
        return false;
    }

    // 启用或禁用写事件：如果 enable 为 true，则将 kEventWrite 标志位添加到事件中；如果为 false，则移除该标志位
    if (enable)
    {
        event->event_ |= kEventWrite;
    }
    else
    {
        event->event_ |= ~kEventWrite;
    }

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.events = event->event_;
    ev.data.fd = event->fd_;
    // 使用 epoll_ctl 函数修改 epoll 中的事件，更新为新的事件设置
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &ev);

    return true;
}

// 根据传入的参数启用或禁用特定事件的读取功能，并更新 epoll 事件
bool EventLoop::EnableEventReading(const EventPtr &event, bool enable)
{
    auto iter = events_.find(event->Fd());
    // 使用 events_ 容器查找与 event->Fd() 相关的事件，如果找不到对应的事件，输出错误信息并返回 false
    if (iter == events_.end())
    {
        NETWORK_ERROR << "Cannot find event fd: " << event->Fd();
        
        return false;
    }

    // 启用或禁用读事件：如果 enable 为 true，则将 kEventRead 标志位添加到事件中；如果为 false，则移除该标志位
    if (enable)
    {
        event->event_ |= kEventRead;
    }
    else
    {
        event->event_ |= ~kEventRead;
    }

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));
    ev.events = event->event_;
    ev.data.fd = event->fd_;
    // 调用 epoll_ctl 函数，使用 EPOLL_CTL_MOD 操作来修改指定文件描述符的事件
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &ev);

    return true;
}