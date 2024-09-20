#include "LiveService.h"
#include "base/StringUtils.h"
#include "live/base/LiveLog.h"
#include "mmedia/rtmp/RtmpServer.h"
#include "base/Config.h"
#include "Session.h"
#include "base/TTime.h"
#include "Stream.h"

using namespace lss::live;
using namespace lss::mm;

// 定义一个匿名命名空间，限制内部符号的可见性
namespace
{
    // 声明一个静态智能指针 session_null，用于表示空会话
    static SessionPtr session_null;
}

SessionPtr LiveService::CreateSession(const std::string &session_name)
{
    // 创建一个锁的作用域，以保护共享资源 lock_
    std::lock_guard<std::mutex> lk(lock_);

    // 在会话映射中查找指定名称的会话
    auto iter = sessions_.find(session_name);

    // 如果找到了会话
    if (iter != sessions_.end())
    {
        // 返回已存在的会话智能指针
        return iter->second;
    }

    // 将会话名称按斜杠分割成字符串列表
    auto list = base::StringUtils::SplitString(session_name, "/");

    // 如果分割后的列表大小不是3
    if (list.size() != 3)
    {
        // 记录错误日志，表示会话名称无效
        LIVE_ERROR << " create session failed. Invalid session name : " << session_name;
        
        // 返回空会话指针
        return session_null;
    }
    
    // 获取配置管理器中的配置
    ConfigPtr config = configManager->GetConfig();

    // 获取应用信息，使用分割后的前两个元素
    auto app_info = config->GetAppInfo(list[0], list[1]);

    // 如果未找到应用信息
    if (!app_info)
    {
        // 记录错误日志，表示未找到配置
        LIVE_ERROR << " create session failed. cant found config. domain : " << list[0] << " app : " << list[1];
        
        // 返回空会话指针
        return session_null;
    }

    // 创建一个新的 Session 对象的智能指针
    auto s = std::make_shared<Session>(session_name);

    // 设置 Session 对象的应用信息
    s->SetAppInfo(app_info);

    // 将新创建的会话加入到会话映射中
    sessions_.emplace(session_name,s);

    // 记录调试日志，表示会话创建成功，输出会话名称和当前时间
    LIVE_DEBUG << " create session success. session_name : " << session_name << " now : " << base::TTime::NowMS();
    
    // 返回新创建的会话智能指针
    return s;
}

SessionPtr LiveService::FindSession(const std::string &session_name)
{
    // 创建一个锁的作用域，以保护共享资源 lock_
    std::lock_guard<std::mutex> lk(lock_);

    // 在会话映射中查找指定名称的会话
    auto iter = sessions_.find(session_name);

    // 如果找到了会话
    if (iter != sessions_.end())
    {
        // 返回找到的会话智能指针
        return iter->second;
    }

    // 如果未找到会话，返回空会话指针
    return session_null;
}

bool LiveService::CloseSession(const std::string &session_name)
{
    // 声明一个 SessionPtr 类型的智能指针，用于存储要关闭的会话
    SessionPtr s;
    {
        // 创建一个锁的作用域，以保护共享资源 lock_
        std::lock_guard<std::mutex> lk(lock_);

        // 在会话映射中查找指定名称的会话
        auto iter = sessions_.find(session_name);

        // 如果找到了会话
        if (iter != sessions_.end())
        {
            // 获取找到的会话智能指针
            s = iter->second;

            // 从会话映射中移除该会话
            sessions_.erase(iter);
        }
    }

    // 如果成功获取到了会话
    if (s)
    {
        // 记录信息日志，输出关闭的会话名称和当前时间
        LIVE_INFO << " close session : " << s->SessionName()  << " now : " << base::TTime::NowMS();
        
        // 调用会话的清理方法
        s->Clear();
    }

    // 返回关闭会话成功的标志
    return true;
}

void LiveService::OnTimer(const TaskPtr &t)
{
    // 创建一个锁的作用域，以保护共享资源 lock_
    std::lock_guard<std::mutex> lk(lock_);

    // 遍历所有会话
    for (auto iter = sessions_.begin(); iter != sessions_.end();)
    {
        // 如果会话超时
        if (iter->second->IsTimeout())
        {
            // 记录信息日志，输出超时的会话名称和当前时间
            LIVE_INFO << " session : " << iter->second->SessionName() << " is timeout. close it. Now : " << base::TTime::NowMS();

            // 清理超时的会话
            iter->second->Clear();

            // 移除超时的会话，并更新迭代器
            iter = sessions_.erase(iter);
        }
        else
        {
            // 继续遍历下一个会话
            iter ++;
        }
    }

    // 重启定时任务
    t->Restart();
}

void LiveService::OnNewConnection(const TcpConnectionPtr &conn)
{

}

void LiveService::OnConnectionDestroy(const TcpConnectionPtr &conn)
{
    // 从连接上下文中获取用户信息
    auto user = conn->GetContext<User>(kUserContext);
    // 如果找到了用户
    if (user)
    {
        // 关闭与该用户相关的会话
        user->GetSession()->CloseUser(user);
    }
}

void LiveService::OnActive(const ConnectionPtr &conn)
{
    // 从连接上下文中获取玩家用户信息
    auto user = conn->GetContext<PlayerUser>(kUserContext);

    // 如果找到了用户且用户类型满足条件
    if (user && user->GetUserType() >= UserType::kUserTypePlayerPav)
    {
        // 发送用户帧数据
        user->PostFrames();
    }
    // else
    // {
    //     LIVE_ERROR << " no user found. host : " << conn->PeerAddr().ToIpPort();
    //     conn->ForceClose();
    // }
}

bool LiveService::OnPlay(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param)
{
    // 记录调试日志，输出播放的会话名称、参数、连接地址和当前时间
    LIVE_DEBUG << " on play session name : " << session_name 
                << " param : " << param 
                << " host : " << conn->PeerAddr().ToIpPort()
                << " now : " << TTime::NowMS();
    
    // 创建会话，返回会话智能指针
    auto s = CreateSession(session_name);
    
    // 如果会话创建失败
    if (!s)
    {
        // 记录错误日志，表示会话创建失败
        LIVE_ERROR << " create session failed. session name : " << session_name;
        
        // 强制关闭连接
        conn->ForceClose();
        
        // 返回失败
        return false;
    }

    // 创建播放器用户，返回用户智能指针
    auto user = s->CreatePlayerUser(conn, session_name, param, UserType::kUserTypePlayerRtmp);
    
    // 如果用户创建失败
    if (!user)
    {
        // 记录错误日志，表示用户创建失败
        LIVE_ERROR << " create user failed. session name : " << session_name;
        
        // 强制关闭连接
        conn->ForceClose();
        
        // 返回失败
        return false;
    }

    // 将用户上下文设置到连接中
    conn->SetContext(kUserContext, user);

    // 将用户添加到会话的播放器列表中
    s->AddPlayer(std::dynamic_pointer_cast<PlayerUser>(user));

    // 返回成功
    return true;     
}

bool LiveService::OnPublish(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param)
{
    // 记录调试日志，输出发布的会话名称、参数、连接地址和当前时间
    LIVE_DEBUG << " on publish session name : " << session_name 
                << " param : " << param 
                << " host : " << conn->PeerAddr().ToIpPort()
                << " now : " << TTime::NowMS();

    // 创建会话，返回会话智能指针
    auto s = CreateSession(session_name);

    // 如果会话创建失败
    if (!s)
    {
        // 记录错误日志，表示会话创建失败
        LIVE_ERROR << " create session failed. session name : " << session_name;

        // 强制关闭连接
        conn->ForceClose();

        // 返回失败
        return false;
    } 

    // 创建发布用户，返回用户智能指针
    auto user = s->CreatePublishUser(conn, session_name, param, UserType::kUserTypePublishRtmp);
    
    // 如果用户创建失败
    if (!user)
    {
        // 记录错误日志，表示用户创建失败
        LIVE_ERROR << " create user failed. session name : " << session_name;
        
        // 强制关闭连接
        conn->ForceClose();
        
        // 返回失败
        return false;
    }    

    // 将用户上下文设置到连接中
    conn->SetContext(kUserContext, user);

    // 将用户设置为会话的发布者
    s->SetPublisher(user);

    // 返回成功
    return true;
}

void LiveService::OnRecv(const TcpConnectionPtr &conn, PacketPtr &&data) 
{
    // 从连接上下文中获取用户信息
    auto user = conn->GetContext<User>(kUserContext);

    // 如果未找到用户
    if (!user)
    {
        // 记录错误日志，输出未找到用户的连接信息
        LIVE_ERROR << " no found user. host : " << conn->PeerAddr().ToIpPort();

        // 强制关闭连接
        conn->ForceClose();

        // 返回
        return;
    }

    // 将接收到的数据包移动到用户的流中
    user->GetStream()->AddPacket(std::move(data));
}

void LiveService::Start()
{
    // 获取配置管理器中的配置
    ConfigPtr config = configManager->GetConfig();

    // 创建事件循环线程池
    pool_ = new EventLoopThreadPool(config->thread_nums_, config->cpu_start_, config->cpus_);
    
    // 启动线程池
    pool_->Start();

    // 获取服务信息
    auto services = config->GetServiceInfos();

    // 获取事件循环列表
    auto eventloops = pool_->GetLoops();

    // 记录事件循环的数量
    LIVE_TRACE << " eventloops size : " << eventloops.size();

    // 遍历每个事件循环
    for (auto &el : eventloops)
    {
        // 遍历每个服务信息
        for(auto &s : services)
        {
            // 如果服务协议是 RTMP
            if (s->protocol == "RTMP" || s->protocol == "rtmp")
            {
                // 创建本地地址对象
                InetAddress local(s->addr, s->port);

                // 创建 RTMP 服务器实例
                TcpServer *server = new RtmpServer(el, local, this);

                // 将服务器实例添加到服务器列表
                servers_.push_back(server);

                // 启动该服务器
                servers_.back()->Start();
            }
        }
    }
    
    // 创建一个定时任务，定期调用 OnTimer 方法
    TaskPtr t = std::make_shared<Task>(std::bind(&LiveService::OnTimer, this, std::placeholders::_1), 5000);

    // 将定时任务添加到任务管理器中
    taskManager->Add(t);
}

void LiveService::Stop()
{

}

EventLoop *LiveService::GetNextLoop()
{
    // 从线程池中获取下一个事件循环
    return pool_->GetNextLoop();
}