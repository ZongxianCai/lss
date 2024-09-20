#pragma once
#include <string>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include "PlayerUser.h"
#include "User.h"
#include "base/AppInfo.h"

namespace lss
{
    namespace live
    {
        // 使用智能指针定义 PlayerUser 的指针类型
        using PlayerUserPtr = std::shared_ptr<PlayerUser>;

        // 使用智能指针定义 User 的指针类型
        using UserPtr = std::shared_ptr<User>;

        // Session 类，继承 std::enable_shared_from_this，允许对象创建 shared_ptr
        class Session : public std::enable_shared_from_this<Session>
        {
        public:
            // 构造函数，接收 session 名称作为参数，使用 explicit 防止隐式转换
            explicit Session(const std::string &session_name);

            // 获取准备时间，返回值类型为 32 位整数，常量函数表示不会修改成员变量
            int32_t ReadyTime() const;

            // 获取自开始以来的时间，返回值类型为 64 位整数，常量函数
            int64_t SinceStart() const;

            // 检查会话是否超时，返回布尔值
            bool IsTimeout();

            // 创建发布者用户，返回 UserPtr 智能指针，接收连接指针、会话名、参数和用户类型
            UserPtr CreatePublishUser(const ConnectionPtr &conn, const std::string &session_name, const std::string &param, UserType type);

            // 创建播放用户，返回 UserPtr 智能指针，接收连接指针、会话名、参数和用户类型
            UserPtr CreatePlayerUser(const ConnectionPtr &conn, const std::string &session_name, const std::string &param, UserType type);

            // 关闭指定的用户，参数是 UserPtr 类型
            void CloseUser(const UserPtr &user);

            // 激活所有播放用户
            void ActiveAllPlayers();

            // 添加用户，参数是 PlayerUserPtr 类型
            void AddPlayer(const PlayerUserPtr &user);

            // 设置发布者用户，参数是 UserPtr 类型
            void SetPublisher(UserPtr &user);
            
            // 获取流对象，返回 StreamPtr 类型
            StreamPtr GetStream();

            // 获取会话名称，返回字符串常量引用
            const string &SessionName() const;

            // 设置应用程序信息，参数是 AppInfoPtr 智能指针
            void SetAppInfo(AppInfoPtr &ptr);

            // 获取应用程序信息，返回 AppInfoPtr 智能指针的引用
            AppInfoPtr &GetAppInfo();

            // 判断是否正在发布，返回布尔值，常量函数
            bool IsPublishing() const;

            // 清除会话的所有状态
            void Clear();

        private:    
            // 在不加锁的情况下关闭用户，参数是 UserPtr 类型
            void CloseUserNoLock(const UserPtr &user);

            // 会话名称，存储为字符串
            std::string session_name_;

            // 保存玩家用户的集合，使用无序集合存储 PlayerUserPtr
            std::unordered_set<PlayerUserPtr> players_;

            // 应用程序信息指针，使用智能指针类型 AppInfoPtr
            AppInfoPtr app_info_;

            // 流对象指针，使用智能指针类型 StreamPtr
            StreamPtr stream_;

            // 发布者用户指针，使用智能指针类型 UserPtr
            UserPtr publisher_;

            // 互斥锁，用于同步操作
            std::mutex lock_;

            // 原子类型，表示玩家活动时间，64 位整数类型
            std::atomic<int64_t> player_live_time_;
        };
    }
}