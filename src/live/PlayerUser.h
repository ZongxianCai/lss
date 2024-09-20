#pragma once
#include <vector>
#include "User.h"
#include "mmedia/base/Packet.h"
#include "live/base/TimeCorrector.h"

namespace lss
{
    namespace live
    {
        using namespace lss::mm;

        // 定义 PlayerUser 类，继承自 User 类
        class PlayerUser : public User
        {
        public:
            // 声明 Stream 类为友类
            friend class Stream;

            // 构造函数，接受连接指针、流指针和会话指针
            explicit PlayerUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s);

            // 获取元数据的指针
            PacketPtr Meta() const;

            // 获取视频头信息的指针
            PacketPtr VideoHeader() const;

            // 获取音频头信息的指针
            PacketPtr AudioHeader() const;

            // 清除元数据
            void ClearMeta();

            // 清除音频头信息
            void ClearAudioHeader();

            // 清除视频头信息
            void ClearVideoHeader();  

            // 纯虚函数，发布帧
            virtual bool PostFrames() = 0;

            // 获取时间校正器的引用
            TimeCorrector &GetTimeCorrector();

        protected:
            // 视频头信息的指针
            PacketPtr video_header_; 

            // 音频头信息的指针
            PacketPtr audio_header_;

            // 元数据的指针
            PacketPtr meta_;  

            // 是否等待元数据，默认为 true
            bool wait_meta_{true}; 

            // 是否等待音频，默认为 true
            bool wait_audio_{true}; 

            // 是否等待视频，默认为 true
            bool wait_video_{true}; 

            // 视频头索引，默认为 0
            int32_t video_header_index_{0};

            // 音频头索引，默认为 0
            int32_t audio_header_index_{0};

            // 元数据索引，默认为 0
            int32_t meta_index_{0};

            // 时间校正器对象
            TimeCorrector time_corrector_;

            // 是否等待超时，默认为 false
            bool wait_timeout_{false};

            // 输出版本，默认为 -1
            int32_t out_version_{-1};

            // 输出帧时间戳，默认为 0
            int32_t out_frame_timestamp_{0};

            // 输出帧的指针向量
            std::vector<PacketPtr> out_frames_;

            // 输出索引，默认为 -1
            int32_t out_index_{-1};
        };
    }
}