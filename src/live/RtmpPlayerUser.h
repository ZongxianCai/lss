#pragma once
#include "PlayerUser.h"

namespace lss
{
    namespace live
    {
        // 定义 RtmpPlayerUser 类，继承自 PlayerUser
        class RtmpPlayerUser : public PlayerUser
        {
        public:
            // 构造函数，使用智能指针传递连接、流和会话
            explicit RtmpPlayerUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s);

            // 发布帧
            bool PostFrames();
            
            // 获取用户类型
            UserType GetUserType() const;

        private:
            // 使用基类的 SetUserType 方法
            using User::SetUserType;

            // 推送单个帧，带有标头参数
            bool PushFrame(PacketPtr &packet,bool is_header);

            // 推送多个帧
            bool PushFrames(std::vector<PacketPtr> &list);
        };
    }
}