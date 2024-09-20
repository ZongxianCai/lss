#pragma once
#include <cstdint>
#include "mmedia/base/Packet.h"

namespace lss
{
    namespace live
    {
        using namespace lss::mm;

        // TimeCorrector，负责时间戳校正功能，用于音视频同步
        class TimeCorrector
        {
            // 表示视频时间戳的最大允许误差为100毫秒
            const int32_t kMaxVideoDeltaTime = 100;

            // 表示视频时间戳的最大允许误差为100毫秒
            const int32_t kMaxAudioDeltaTime = 100;

            // 表示视频的默认时间戳校正间隔为40毫秒
            const int32_t kDefaultVideoDeltaTime = 40;

            // 表示音频的默认时间戳校正间隔为20毫秒
            const int32_t kDefaultAudioDeltaTime = 20; 

            public:
                // 默认构造函数
                TimeCorrector() = default;
            
                // 校正传入Packet（数据包）中的时间戳，返回校正后的时间戳（以毫秒为单位）
                uint32_t CorrectTimestamp(const PacketPtr &packet);

                // 通过视频时间戳来校正音频的时间戳，返回校正后的时间戳
                uint32_t CorrectAudioTimeStampByVideo(const PacketPtr &packet);

                // 通过视频时间戳校正视频的时间戳，返回校正后的时间戳
                uint32_t CorrectVideoTimeStampByVideo(const PacketPtr &packet);

                // 通过音频时间戳校正音频时间戳，返回校正后的时间戳
                uint32_t CorrectAudioTimeStampByAudio(const PacketPtr &packet);

                // 默认析构函数
                ~TimeCorrector() = default;

            private:
                // 存储视频原始的时间戳，初始化为-1，表示未初始化或无效状态
                int64_t video_original_timestamp_{-1};

                // 存储校正后的视频时间戳，初始值为0
                int64_t video_corrected_timestamp_{0};

                // 存储音频原始的时间戳，初始化为-1，表示未初始化或无效状态
                int64_t audio_original_timestamp_{-1};

                // 存储校正后的音频时间戳，初始值为0
                int64_t audio_corrected_timestamp_{0};

                // 用于记录在视频帧之间的音频帧数量，初始值为0
                int32_t audio_numbers_between_video_{0};
        };
    }
}