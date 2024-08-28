#pragma once
#include <functional>
#include <unordered_map>
#include <memory>
#include <atomic>
#include "network/base/InetAddress.h"
#include "Event.h"
#include "EventLoop.h"

namespace lss
{
    namespace network
    {
        // 列举不同的上下文类型，每个上下文类型都有一个与之关联的整数值，分别为0，1，2，3，4
        enum
        {
            kNormalContext = 0,
            kRtmpContext,
            kHttpContext,
            kUserContext,
            kFlvContext
        };

        // 定义一个类型别名 ContexPtr，表示指向 void 类型的共享指针，通常用于表示不确定类型的上下文
        using ContexPtr = std::shared_ptr<void>;

        // 前向声明 Connection 类，表示该类将在后面定义
        class Connection;

        // 定义一个类型别名 ConnectionPtr，表示指向 Connection 类的共享指针
        using ConnectionPtr = std::shared_ptr<Connection>;

        // 定义一个类型别名 ActiveCallback，表示一个接受 ConnectionPtr 类型参数并返回 void 的函数类型
        using ActiveCallback = std::function<void(const ConnectionPtr&)>;

        class Connection : public Event
        {
        public:
            // 声明 Connection 类的构造函数，接受一个事件循环指针、文件描述符和两个 InetAddress 类型的参数，分别表示本地地址和远端地址
            Connection(EventLoop *loop, int fd, const InetAddress &localAddr, const InetAddress &peerAddr);

            // 声明一个成员函数 SetLocalAddr，用于设置本地地址
            void SetLocalAddr(const InetAddress &local);

            // 声明一个成员函数 SetPeerAddr，用于设置远端地址
            void SetPeerAddr(const InetAddress &peer);

            // 获取本地地址的常量引用
            const InetAddress &LocalAddr() const;

            // 获取远端地址的常量引用
            const InetAddress &PeerAddr() const;

            // 声明一个函数，用于设置特定类型的上下文，接受一个类型和一个共享指针作为参数
            void SetContext(int type, const std::shared_ptr<void> &context);

            // 声明一个函数，用于设置特定类型的上下文，接受一个类型和一个右值引用的共享指针作为参数
            void SetContext(int type, std::shared_ptr<void> &&context);

            // 声明一个模板函数，用于获取特定类型的上下文，接受一个类型参数，返回一个共享指针
            template <typename T> 
            std::shared_ptr<T> GetContext(int type) const
            {
                // 在 contexts_ 中查找特定类型的上下文
                auto iter = contexts_.find(type);

                // 如果找到对应类型的上下文
                if (iter != contexts_.end())
                {
                    // 将上下文转换为指定类型的共享指针并返回
                    return std::dynamic_pointer_cast<T>(iter->second);
                }

                // 如果未找到对应类型的上下文，则返回空的共享指针
                return std::shared_ptr<T>();
            }

            // 声明一个函数，用于清除特定类型的上下文，接受一个类型作为参数
            void ClearContext(int type);

            // 声明一个函数，用于清除所有上下文
            void ClearContext();

            // 声明一个函数，用于设置活动回调函数，接受一个 ActiveCallback 类型的参数
            void SetActiveCallback(const ActiveCallback &cb);

            // 声明一个函数，用于设置活动回调函数，接受一个右值引用的 ActiveCallback 类型的参数
            void SetActiveCallback(ActiveCallback &&cb);

            // 声明一个函数，用于激活连接
            void Active();

            // 声明一个函数，用于停用连接
            void Deactive();

            // 声明一个虚函数，用于强制关闭连接，子类必须实现该函数
            virtual void ForceClose() = 0;

            // 虚析构函数，使用默认实现
            virtual ~Connection() = default;

        protected:
            // 本地地址
            InetAddress local_addr_;

            // 远端地址
            InetAddress peer_addr_;

        private:
            // 存储上下文的哈希表，键为整数，值为上下文指针
            std::unordered_map<int, ContexPtr> contexts_;

            // 激活回调函数
            ActiveCallback active_cb_;

            // 表示当前是否处于活动状态的原子布尔值
            std::atomic<bool> active_{false};
        };
    }
}