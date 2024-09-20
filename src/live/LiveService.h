#pragma once
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>
#include "network/net/Connection.h"
#include "network/TcpServer.h"
#include "network/net/EventLoopThreadPool.h"
#include "base/TaskManager.h"
#include "base/Task.h"
#include "base/NonCopyable.h"
#include "base/Singleton.h"
#include "mmedia/rtmp/RtmpHandler.h"

namespace lss
{
    namespace live
    {
        using namespace lss::network;
        using namespace lss::base;
        using namespace lss::mm;
        
        // 前向声明 Session 类
        class Session;

        // 使用智能指针管理 Session 对象的生命周期
        using SessionPtr = std::shared_ptr<Session>;

        // LiveService 类，继承自 RtmpHandler，用于处理 RTMP 协议的会话服务
        class LiveService : public RtmpHandler
        {
        public:
            // 默认构造函数
            LiveService() = default;

            // 创建一个新的会话，传入会话名称，返回会话的智能指针
            SessionPtr CreateSession(const std::string &session_name);

            // 查找已有的会话，传入会话名称，返回会话的智能指针
            SessionPtr FindSession(const std::string &session_name);

            // 关闭指定名称的会话，返回是否成功
            bool CloseSession(const std::string &session_name); 

            // 定时器触发的回调函数，处理特定的定时任务
            void OnTimer(const TaskPtr &t);

            // 当有新的 TCP 连接时的回调函数
            void OnNewConnection(const TcpConnectionPtr &conn) override;

            // 当 TCP 连接销毁时的回调函数
            void OnConnectionDestroy(const TcpConnectionPtr &conn) override;

            // 当连接激活时的回调函数
            void OnActive(const ConnectionPtr &conn) override;

            // 处理播放请求，传入连接、会话名称和参数，返回是否处理成功
            bool OnPlay(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) override;

            // 处理发布请求，传入连接、会话名称和参数，返回是否处理成功
            bool OnPublish(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) override;

            // 处理接收的数据，传入连接和数据包的右值引用
            void OnRecv(const TcpConnectionPtr &conn, PacketPtr &&data) override;

            // 处理接收的数据，传入连接和数据包的引用
            void OnRecv(const TcpConnectionPtr &conn, const PacketPtr &data) override{};
            
            // 启动直播服务
            void Start();

            // 停止直播服务
            void Stop();

            // 获取下一个事件循环，用于处理不同的连接
            EventLoop *GetNextLoop();

            // 默认析构函数
            ~LiveService() = default;

        private:
            // 事件循环线程池，用于管理多个事件循环
            EventLoopThreadPool * pool_{nullptr};

            // 保存所有的 TCP 服务器实例
            std::vector<TcpServer*> servers_;

            // 互斥锁，用于保护共享资源
            std::mutex lock_;

            // 使用哈希表保存所有会话，键为会话名称，值为会话的智能指针
            std::unordered_map<std::string, SessionPtr> sessions_;
        };

        // 定义宏 sLiveService，用于获取 LiveService 的单例实例
        #define sLiveService lss::base::Singleton<lss::live::LiveService>::Instance()
    }
}