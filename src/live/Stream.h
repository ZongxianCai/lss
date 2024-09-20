#pragma once
#include <string>
#include <memory>
#include <cstdint>
#include <atomic>
#include <vector>
#include <mutex>
#include "live/base/TimeCorrector.h"
#include "live/GopMgr.h"
#include "live/CodecHeader.h"
#include "mmedia/base/Packet.h"
#include "PlayerUser.h"
#include "User.h"

namespace lss
{
    namespace live
    {
        using namespace lss::mm;

        // 定义 UserPtr 为 User 的智能指针
        using UserPtr = std::shared_ptr<User>;

        // 定义 PlayerUserPtr 为 PlayerUser 的智能指针
        using PlayerUserPtr = std::shared_ptr<PlayerUser>;

        // 前向声明 Session 类
        class Session;

        class Stream
        {
        public:
            // 构造函数，接受 Session 引用和会话名称
            Stream(Session &s, const std::string &session_name);

            // 获取准备时间
            int64_t ReadyTime() const;

            // 获取自开始以来的时间
            int64_t SinceStart() const;

            // 检查是否超时
            bool Timeout();

            // 获取数据时间
            int64_t DataTime() const;

            // 获取会话名称的引用
            const std::string &SessionName() const;

            // 获取流版本
            int32_t StreamVersion() const;

            // 检查是否有媒体
            bool HasMedia() const;

            // 检查流是否准备好
            bool Ready() const;

            // 添加数据包
            void AddPacket(PacketPtr &&packet);

            // 获取帧数据给指定用户
            void GetFrames(const PlayerUserPtr &user);

        private:
            // 定位 GOP（图像组）给指定用户
            bool LocateGop(const PlayerUserPtr &user);

            // 跳过帧给指定用户
            void SkipFrame(const PlayerUserPtr &user);

            // 获取下一帧给指定用户
            void GetNextFrame(const PlayerUserPtr &user); 

            // 设置流的准备状态
            void SetReady(bool ready);

            // 数据到达时间，初始化为 0
            int64_t data_coming_time_{0};

            // 流开始时间戳，初始化为 0
            int64_t start_timestamp_{0};

            // 准备时间，初始化为 0
            int64_t ready_time_{0};

            // 流时间，使用原子变量，初始化为 0
            std::atomic<int64_t> stream_time_{0};

            // 引用 Session 对象
            Session &session_;

            // 会话名称字符串
            std::string session_name_;

            // 帧索引，使用原子变量，初始化为 -1
            std::atomic<int64_t> frame_index_{-1}; 

            // 数据包缓冲区大小，初始化为 1000
            uint32_t packet_buffer_size_{1000};

            // 数据包缓冲区
            std::vector<PacketPtr> packet_buffer_;

            // 是否有音频，初始化为 false
            bool has_audio_{false};

            // 是否有视频，初始化为 false
            bool has_video_{false};

            // 是否有元数据，初始化为 false
            bool has_meta_{false};

            // 流是否准备好，初始化为 false
            bool ready_{false};

            // 流版本，使用原子变量，初始化为 -1
            std::atomic<int32_t> stream_version_{-1};

            // GOP 管理器
            GopMgr gop_mgr_;

            // 编解码头
            CodecHeader codec_headers_;

            // 时间校正器
            TimeCorrector time_corrector_;

            // 互斥锁，用于线程同步
            std::mutex lock_;
        };
    }
}