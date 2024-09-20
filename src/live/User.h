#pragma once
#include <memory>
#include <cstdint>
#include <string>
#include <atomic>
#include "network/net/Connection.h"
#include "base/AppInfo.h"

namespace lss
{
    namespace live
    {
        using namespace lss::network;
        using namespace lss::base;

        using std::string;

        // 定义 AppInfoPtr 为 AppInfo 的共享指针
        using AppInfoPtr = std::shared_ptr<AppInfo>;

        // 前向声明 Session 类
        class Session;

        // 定义 SessionPtr 为 Session 的共享指针
        using SessionPtr = std::shared_ptr<Session>;

        // 定义用户类型枚举
        enum class UserType
        {
            kUserTypePublishRtmp = 0,  // 发布 RTMP 类型
            kUserTypePublishMpegts,     // 发布 MPEG-TS 类型
            kUserTypePublishPav,        // 发布 PAV 类型
            kUserTypePublishWebRtc,     // 发布 WebRTC 类型
            kUserTypePlayerPav,         // 播放 PAV 类型
            kUserTypePlayerFlv,         // 播放 FLV 类型
            kUserTypePlayerHls,         // 播放 HLS 类型
            kUserTypePlayerRtmp,        // 播放 RTMP 类型
            kUserTypePlayerWebRTC,      // 播放 WebRTC 类型
            kUserTypeUnknowed = 255     // 未知用户类型
        };

        // 定义用户协议枚举
        enum class UserProtocol
        {
            kUserProtocolHttp = 0,     // HTTP 协议
            kUserProtocolHttps,         // HTTPS 协议
            kUserProtocolQuic,          // QUIC 协议
            kUserProtocolRtsp,          // RTSP 协议
            kUserProtocolWebRTC,        // WebRTC 协议
            kUserProtocolUdp,           // UDP 协议
            kUserProtocolUnknowed = 255  // 未知协议
        };

        // 前向声明 Stream 类
        class Stream;

        // 定义 StreamPtr 为 Stream 的共享指针
        using StreamPtr = std::shared_ptr<Stream>;

        // 用户类，继承自 std::enable_shared_from_this<User>
        class User : public std::enable_shared_from_this<User>
        {
        public:
            // 让 Session 类成为友元类
            friend class Session;

            // 构造函数，接受连接指针、流指针和会话指针
            explicit User(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s);

            // 获取域名
            const string &DomainName() const;

            // 设置域名
            void SetDomainName(const string &domain);

            // 获取应用名称
            const string &AppName() const;

            // 设置应用名称
            void SetAppName(const string &domain);

            // 获取流名称
            const string &StreamName() const;

            // 设置流名称
            void SetStreamName(const string &domain);

            // 获取参数
            const string &Param() const;

            // 设置参数
            void SetParam(const string &domain);   

            // 获取应用信息指针
            const AppInfoPtr &GetAppInfo() const;

            // 设置应用信息指针
            void SetAppInfo(const AppInfoPtr &info);

            // 获取用户类型
            virtual UserType GetUserType() const;

            // 设置用户类型
            void SetUserType(UserType t);

            // 获取用户协议
            virtual UserProtocol GetUserProtocol() const;

            // 设置用户协议
            void SetUserProtocol(UserProtocol p);
            
            // 关闭用户连接
            void Close();

            // 获取连接指针
            ConnectionPtr GetConnection();

            // 获取经过的时间
            uint64_t ElapsedTime();

            // 激活用户
            void Active();

            // 注销用户
            void Deactive();

            // 获取用户 ID
            const std::string &UserId() const 
            {
                return user_id_;
            }

            // 获取会话指针
            SessionPtr GetSession() const 
            {
                return session_;
            }

            // 获取流指针
            StreamPtr GetStream() const 
            {
                return stream_;
            }

            // 虚析构函数
            virtual ~User() = default;

        protected:

            // 连接指针
            ConnectionPtr connection_;

            // 流指针
            StreamPtr stream_;

            // 域名
            string domain_name_;

            // 应用名称
            string app_name_;

            // 流名称
            string stream_name_;

            // 参数
            string param_;

            // 用户 ID
            string user_id_;

            // 应用信息指针
            AppInfoPtr app_info_;

            // 开始时间戳
            int64_t start_timestamp_{0};

            // 用户类型
            UserType type_{UserType::kUserTypeUnknowed};

            // 用户协议
            UserProtocol protocol_{UserProtocol::kUserProtocolUnknowed};

            // 是否被销毁标志
            std::atomic_bool destroyed_{false};

            // 会话指针
            SessionPtr session_;
        };
    }
}