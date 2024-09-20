#include "Session.h"
#include "Stream.h"
#include "base/TTime.h"
#include "base/AppInfo.h"
#include "live/base/LiveLog.h"
#include "base/StringUtils.h"
#include "live/RtmpPlayerUser.h"

using namespace lss::live;
using namespace lss::base;

namespace
{
    // 在匿名命名空间中定义一个静态变量 user_null，类型为 UserPtr (智能指针类型)
    // 使用 static 关键字意味着该变量仅在当前编译单元中可见，避免外部访问或重复定义
    static UserPtr user_null;
}

// 构造函数，使用初始化列表将 session_name_ 初始化为传入的 session_name
Session::Session(const std::string &session_name)
    : session_name_(session_name)
{
    // 创建 Stream 对象并将其存储在 stream_ 中，传入当前 Session 对象和 session_name
    stream_ = std::make_shared<Stream>(*this, session_name);

    // 初始化 player_live_time_ 为当前时间（毫秒），调用 TTime::NowMS() 获取当前时间戳
    player_live_time_ = TTime::NowMS();
}

int32_t Session::ReadyTime() const
{
    // 返回 stream_ 的 ReadyTime() 方法的结果，表示流的准备时间
    // 该方法是 const，保证不会修改成员变量
    return stream_->ReadyTime();
}

int64_t Session::SinceStart() const
{
    // 返回 stream_ 的 SinceStart() 方法的结果，表示自流开始以来的时间
    // 该方法是 const，保证不会修改成员变量
    return stream_->SinceStart();
}

bool Session::IsTimeout()
{
    // 如果流的 Timeout() 方法返回 true，说明超时，直接返回 true
    if (stream_->Timeout())
    {
        return true;
    }

    // 计算自最后一次玩家活动到现在的空闲时间 (毫秒)
    auto idle = TTime::NowMS() - player_live_time_;

    // 如果没有玩家并且空闲时间超过了应用设定的流空闲时间，则返回 true，表示超时
    if (players_.empty() && idle > app_info_->stream_idle_time)
    {
        return true;
    }

    // 如果不满足超时条件，则返回 false
    return false;
}

UserPtr Session::CreatePublishUser(const ConnectionPtr &conn, const std::string &session_name, const std::string &param, UserType type)
{
    // 如果传入的会话名称不匹配当前会话的名称，打印错误日志并返回 user_null
    if (session_name != session_name_)
    {
        LIVE_ERROR << " create publish user failed. Invalid session name : " << session_name;

        return user_null;
    }

    // 使用 StringUtils 将会话名称按 '/' 分割成列表
    auto list = base::StringUtils::SplitString(session_name, "/");

    // 如果分割后的列表长度不是 3，打印错误日志并返回 user_null
    if (list.size() != 3)
    {
        LIVE_ERROR << " create publish user failed. Invalid session name : " << session_name;
        
        return user_null;
    }

    // 使用 std::make_shared 创建一个新的 User 对象，传入连接对象、流对象以及当前会话的 shared_ptr
    // shared_from_this() 用于在类中获取当前对象的 shared_ptr
    UserPtr user = std::make_shared<User>(conn, stream_, shared_from_this());

    // 设置用户的应用程序信息
    user->SetAppInfo(app_info_);

    // 设置用户的域名为分割后的列表的第一个元素
    user->SetDomainName(list[0]);

    // 设置用户的应用名称为分割后的列表的第二个元素
    user->SetAppName(list[1]);

    // 设置用户的流名称为分割后的列表的第三个元素
    user->SetStreamName(list[2]);

    // 设置用户的参数
    user->SetParam(param);

    // 设置用户的类型
    user->SetUserType(type);

    // 在连接对象中设置上下文，将用户对象与连接相关联
    conn->SetContext(kUserContext, user);

    // 返回创建的用户对象
    return user;
}

UserPtr Session::CreatePlayerUser(const ConnectionPtr &conn, const std::string &session_name, const std::string &param, UserType type)
{
    // 如果传入的会话名称与当前会话的名称不匹配，打印错误日志并返回 user_null
    if (session_name != session_name_)
    {
        LIVE_ERROR << " create publish user failed. Invalid session name : " << session_name;
        
        return user_null;
    }

    // 使用 StringUtils 工具将会话名称按 '/' 分割成列表
    auto list = base::StringUtils::SplitString(session_name, "/");

    // 如果分割后的列表长度不为 3，打印错误日志并返回 user_null
    if (list.size() != 3)
    {
        LIVE_ERROR << " create publish user failed. Invalid session name : " << session_name;
        
        return user_null;
    }

    // 定义 PlayerUserPtr 类型的用户指针，之后会根据用户类型来初始化
    PlayerUserPtr user;

    // 如果用户类型是 RTMP 播放用户，则创建一个 RtmpPlayerUser 对象
    if (type == UserType::kUserTypePlayerRtmp)
    {
        user = std::make_shared<RtmpPlayerUser>(conn, stream_, shared_from_this());
    }

    // 设置用户的应用程序信息
    user->SetAppInfo(app_info_);

    // 设置用户的域名为分割后的列表的第一个元素
    user->SetDomainName(list[0]);

    // 设置用户的应用名称为分割后的列表的第二个元素
    user->SetAppName(list[1]);

    // 设置用户的流名称为分割后的列表的第三个元素
    user->SetStreamName(list[2]);

    // 设置用户的参数
    user->SetParam(param);

    // 设置用户的类型
    user->SetUserType(type);

    // 在连接对象中设置上下文，将用户对象与连接相关联
    conn->SetContext(kUserContext, user);

    // 返回创建的用户对象
    return user;
}

void Session::CloseUser(const UserPtr &user)
{
    // 使用 atomic 的 exchange 方法将 destroyed_ 标志设置为 true，
    // 如果之前没有被设置过（返回 false），则继续执行关闭操作
    if (!user->destroyed_.exchange(true))
    {
        {
            // 使用 std::lock_guard 对互斥锁加锁，确保线程安全
            std::lock_guard<std::mutex> lk(lock_);

            // 如果用户类型小于等于 WebRTC 播放器类型，且当前会话有发布者
            // 则认为这是一个发布者，需要移除该发布者
            if (user->GetUserType() <= UserType::kUserTypePlayerWebRTC)
            {
                if (publisher_)
                {
                    // 输出调试信息，记录移除发布者的操作，包括会话名、用户ID、用户的已用时间、准备时间和流时间
                    LIVE_DEBUG << " remove publisher, session name : " << session_name_
                                << " , user : " << user->UserId()
                                << " , elapsed : " << user->ElapsedTime()
                                << " , ReadyTime : " << ReadyTime()
                                << " , stream time : " << SinceStart();

                    // 重置 publisher_ 指针，移除发布者
                    publisher_.reset();
                }
            }
            else    // 如果用户类型大于 WebRTC 播放器类型，认为这是一个播放用户
            {
                // 输出调试信息，记录移除玩家的操作，包括会话名、用户ID、用户的已用时间、准备时间和流时间
                LIVE_DEBUG << " remove player, session name : " << session_name_
                            << " , user : " << user->UserId()
                            << " , elapsed : " << user->ElapsedTime()
                            << " , ReadyTime : " << ReadyTime()
                            << " , stream time : " << SinceStart();

                // 从 players_ 集合中移除该播放用户，使用 dynamic_pointer_cast 进行类型转换
                players_.erase(std::dynamic_pointer_cast<PlayerUser>(user));

                // 更新最后一次玩家活动时间为当前时间
                player_live_time_ = lss::base::TTime::NowMS();
            }
        }

        // 调用用户的 Close() 方法，执行用户关闭操作
        user->Close();
    }
}

void Session::ActiveAllPlayers()
{
    // 使用 std::lock_guard 对互斥锁加锁，确保线程安全
    std::lock_guard<std::mutex> lk(lock_);

    // 遍历 players_ 集合中的每个用户，并调用他们的 Active() 方法，激活所有用户
    for (auto const &u : players_)
    {
        u->Active();
    }
}

void Session::AddPlayer(const PlayerUserPtr &user)
{
    {
        // 使用 std::lock_guard 对互斥锁加锁，确保线程安全
        std::lock_guard<std::mutex> lk(lock_);

        // 将传入的播放用户添加到 players_ 集合中
        players_.insert(user);
    }

    // 输出调试信息，记录添加玩家的操作，包括会话名和用户ID
    LIVE_DEBUG << " add player, session name : " << session_name_ << " , user : " << user->UserId();

    // 如果当前没有发布者用户，暂时不做处理
    if (!publisher_)
    {

    }

    // 调用用户的 Active() 方法，激活该播放用户
    user->Active();
}

void Session::SetPublisher(UserPtr &user)
{
    // 使用 std::lock_guard 对互斥锁加锁，确保线程安全
    std::lock_guard<std::mutex> lk(lock_);

    // 如果当前发布者已经是传入的用户，则直接返回，不做任何修改
    if (publisher_ == user)
    {
        return;
    }

    // 如果当前发布者存在并且还没有被标记为销毁，则将其关闭
    if (publisher_ && !publisher_->destroyed_.exchange(true))
    {
        publisher_->Close();
    }

    // 设置新的发布者
    publisher_ = user;
}

StreamPtr Session::GetStream()
{
    // 返回会话中的流对象的指针
    return stream_;
}

const string &Session::SessionName() const
{
    // 返回会话的名称，const 保证不会修改 session_name_
    return session_name_;
}

void Session::SetAppInfo(AppInfoPtr &ptr)
{
    // 设置会话的应用信息
    app_info_ = ptr;
}

AppInfoPtr &Session::GetAppInfo()
{
    // 返回会话的应用信息的指针
    return app_info_;
}

bool Session::IsPublishing() const 
{
    // 检查当前会话是否有发布者，返回 true 表示正在发布，false 表示没有发布者
    // 使用双重否定 (!!) 将智能指针转换为布尔值
    return !!publisher_;
}

void Session::Clear()
{
    // 使用 std::lock_guard 对互斥锁加锁，确保线程安全
    std::lock_guard<std::mutex> lk(lock_);

    // 如果当前有发布者用户，则将其关闭
    if (publisher_)
    {
        CloseUserNoLock(publisher_);
    }

    // 遍历 players_ 集合中的所有玩家，并将每个玩家关闭
    // 使用 dynamic_pointer_cast 进行类型转换
    for (auto const &p : players_)
    {
        CloseUserNoLock(std::dynamic_pointer_cast<User>(p));
    }

    // 清空 players_ 集合，移除所有播放用户
    players_.clear();
}

void Session::CloseUserNoLock(const UserPtr &user)
{
    // 使用 atomic 的 exchange 方法将 destroyed_ 标志设置为 true，如果之前没有被设置过（返回 false），继续执行关闭操作
    if (!user->destroyed_.exchange(true))
    {
        {
            // 如果用户类型小于等于 WebRTC 播放器类型，表示这是发布者用户
            if (user->GetUserType() <= UserType::kUserTypePlayerWebRTC)
            {
                // 如果当前会话有发布者
                if (publisher_)
                {
                    // 输出调试信息，记录移除发布者的操作，包括会话名、用户ID、用户的已用时间、准备时间和流时间
                    LIVE_DEBUG << " remove publisher, session name : " << session_name_
                                << " , user : " << user->UserId()
                                << " , elapsed : " << user->ElapsedTime()
                                << " , ReadyTime : " << ReadyTime()
                                << " , stream time : " << SinceStart();

                    // 关闭该发布者用户
                    user->Close();

                    // 重置发布者指针，移除发布者
                    publisher_.reset();
                }
            }
            else    // 否则表示这是一个播放用户
            {
                // 输出调试信息，记录移除玩家的操作，包括会话名、用户ID、用户的已用时间、准备时间和流时间
                LIVE_DEBUG << " remove player, session name : " << session_name_
                            << " , user : " << user->UserId()
                            << " , elapsed : " << user->ElapsedTime()
                            << " , ReadyTime : " << ReadyTime()
                            << " , stream time : " << SinceStart();

                // 从 players_ 集合中移除该播放用户，使用 dynamic_pointer_cast 进行类型转换         
                players_.erase(std::dynamic_pointer_cast<PlayerUser>(user));
                
                // 关闭该播放用户
                user->Close();

                // 更新最后一次用户直播时间为当前时间
                player_live_time_ = lss::base::TTime::NowMS();
            }
        }
    }
}