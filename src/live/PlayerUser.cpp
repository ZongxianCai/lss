#include "PlayerUser.h"

using namespace lss::live;

PlayerUser::PlayerUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s)
    : User(ptr, stream, s)              // 初始化基类 User
{

}

PacketPtr PlayerUser::Meta() const
{
    // 返回元数据指针
    return meta_;
}

PacketPtr PlayerUser::VideoHeader() const
{
    // 返回视频头指针
    return video_header_;
}

PacketPtr PlayerUser::AudioHeader() const
{
    // 返回音频头指针
    return audio_header_;
}

void PlayerUser::ClearMeta()
{
    // 重置元数据指针
    meta_.reset();
}

void PlayerUser::ClearAudioHeader()
{
    // 重置音频头指针
    audio_header_.reset();
}

void PlayerUser::ClearVideoHeader()
{
    // 重置视频头指针
    video_header_.reset();
}         

TimeCorrector &PlayerUser::GetTimeCorrector()
{
    // 返回时间校正器的引用
    return time_corrector_;
}