#include "TimeCorrector.h"
#include "CodecUtils.h"
#include "live/base/LiveLog.h"

using namespace lss::live;

uint32_t TimeCorrector::CorrectTimestamp(const PacketPtr &packet)
{
    // 检查包是否是编解码头
    if (!CodecUtils::IsCodecHeader(packet))
    {
        // 记录时间戳和包大小
        // LIVE_TRACE << "ts:" << packet->TimeStamp() << " size:" << packet->PacketSize();

        // 获取包类型
        int32_t pt = packet->PacketType();

        // 如果是视频包
        if (pt == kPacketTypeVideo)
        {
            // 校正视频时间戳
            return CorrectVideoTimeStampByVideo(packet);
        }
        // 如果是音频包
        else if(pt == kPacketTypeAudio)
        {
            // 校正音频时间戳
            return CorrectAudioTimeStampByVideo(packet);
        }
    }

    // 如果不是视频或音频包，返回0
    return 0;
}

uint32_t TimeCorrector::CorrectAudioTimeStampByVideo(const PacketPtr &packet)
{
    // 增加视频之间的音频包数量
    ++audio_numbers_between_video_;

    // 如果音频包数量大于1
    if (audio_numbers_between_video_ > 1)
    {
        // 根据音频校正时间戳
        return CorrectAudioTimeStampByAudio(packet);
    }

    // 获取音频包时间戳
    int64_t time = packet->TimeStamp();

    // 如果音频原始时间戳未初始化
    if (video_original_timestamp_ == -1)
    {
        // 初始化音频原始时间戳
        audio_original_timestamp_ = time;
        // 初始化音频校正时间戳
        audio_corrected_timestamp_ = time;

        // 返回当前时间戳
        return time;
    }

    // 计算时间差
    int64_t delta = time - video_original_timestamp_;

    // 判断时间差是否在合理范围内
    bool fine = (delta>-kMaxVideoDeltaTime)&&(delta<kMaxVideoDeltaTime);

    // 判断时间差是否在合理范围内
    if (!fine)
    {
        // 使用默认时间差
        delta = kDefaultVideoDeltaTime;
    }

    // 更新音频原始时间戳
    audio_original_timestamp_ = time;
    // 计算校正时间戳
    audio_corrected_timestamp_ = video_corrected_timestamp_ + delta;

    // 如果校正时间戳小于0，设为0
    if (audio_corrected_timestamp_ < 0)
    {
        audio_corrected_timestamp_ = 0;
    }

    // 返回校正后的音频时间戳
    return audio_corrected_timestamp_;
}

uint32_t TimeCorrector::CorrectVideoTimeStampByVideo(const PacketPtr &packet)
{
    // 重置音频包数量
    audio_numbers_between_video_ = 0;

    // 获取视频包时间戳
    int64_t time = packet->TimeStamp();

    // 如果视频原始时间戳未初始化
    if (video_original_timestamp_ == -1)
    {
        // 初始化视频原始时间戳
        video_original_timestamp_ = time;

        // 初始化视频校正时间戳
        video_corrected_timestamp_ = time;

        // 如果音频原始时间戳已初始化
        if (audio_original_timestamp_ != -1)
        {
            // 计算时间差
            int32_t delta = audio_original_timestamp_ - video_original_timestamp_;

            // 如果时间差超出范围
            if (delta <= -kMaxVideoDeltaTime || delta >= kMaxVideoDeltaTime)
            {
                // 同步视频原始时间戳
                video_original_timestamp_ = audio_original_timestamp_;
                // 同步视频校正时间戳
                video_corrected_timestamp_ = audio_corrected_timestamp_;
            }
        }
    }

    // 同步视频校正时间戳
    int64_t delta = time - video_original_timestamp_;

    // 判断时间差是否在合理范围内
    bool fine = (delta > -kMaxVideoDeltaTime) && (delta < kMaxVideoDeltaTime);

    // 如果不在范围内
    if (!fine)
    {
        // 使用默认时间差
        delta = kDefaultVideoDeltaTime;
    }

    // 更新视频原始时间戳
    video_original_timestamp_ = time;

    // 计算校正后的时间戳
    video_corrected_timestamp_ += delta;

    // 如果校正时间戳小于0，设为0
    if (video_corrected_timestamp_ < 0)
    {
        video_corrected_timestamp_ = 0;
    }

    // 返回校正后的视频时间戳
    return video_corrected_timestamp_;
}

uint32_t TimeCorrector::CorrectAudioTimeStampByAudio(const PacketPtr &packet)
{
    // 获取音频包时间戳
    int64_t time = packet->TimeStamp();

    // 如果音频原始时间戳未初始化
    if (audio_original_timestamp_ == -1)
    {
        // 初始化音频原始时间戳
        audio_original_timestamp_ = time;
        // 初始化音频校正时间戳
        audio_corrected_timestamp_ = time;

        // 返回当前时间戳
        return time;
    }

    // 计算时间差
    int64_t delta = time - audio_original_timestamp_;

    // 判断时间差是否在合理范围内
    bool fine = (delta > -kMaxAudioDeltaTime) && (delta < kMaxAudioDeltaTime);

    // 如果不在范围内
    if (!fine)
    {
        // 使用默认时间差
        delta = kDefaultAudioDeltaTime;
    }

    // 更新音频原始时间戳
    audio_original_timestamp_ = time;

    // 计算校正后的音频时间戳
    audio_corrected_timestamp_ += delta;

    // 如果校正时间戳小于0，设为0
    if (audio_corrected_timestamp_ < 0)
    {
        audio_corrected_timestamp_ = 0;
    }
    
    // 返回校正后的音频时间戳
    return audio_corrected_timestamp_;
}