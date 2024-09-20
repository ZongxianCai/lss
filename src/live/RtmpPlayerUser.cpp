#include "RtmpPlayerUser.h"
#include "Stream.h"
#include "mmedia/rtmp/RtmpContext.h"
#include "live/base/LiveLog.h"
#include "base/TTime.h"

using namespace lss::live;
using namespace lss::mm;

RtmpPlayerUser::RtmpPlayerUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s)
    : PlayerUser(ptr, stream, s)        // 初始化基类 PlayerUser，传递连接、流和会话的智能指针
{

}

bool RtmpPlayerUser::PostFrames()
{
    // 检查流是否准备好以及是否有媒体
    if (!stream_->Ready() || !stream_->HasMedia())
    {
        // 如果流未准备好或没有媒体，返回 false
        return false;
    }

    // 从流中获取帧，使用动态类型转换将当前对象转换为 PlayerUser
    stream_->GetFrames(std::dynamic_pointer_cast<PlayerUser>(shared_from_this()));
    
    // 如果存在元数据
    if (meta_)
    {
        // 推送元数据帧，标头参数为 true
        auto ret = PushFrame(meta_, true);

        // 如果推送成功
        if (ret)
        {
            // 记录日志
            LIVE_INFO << " rtmp sent meta now : " << base::TTime::NowMS() << " host : " << user_id_;
            
            // 重置元数据
            meta_.reset();
        }
    }
    // 如果没有元数据但有音频头
    else if (audio_header_)
    {
        // 推送音频头帧，标头参数为 true
        auto ret = PushFrame(audio_header_, true);

        // 如果推送成功
        if (ret)
        {
            // 记录日志
            LIVE_INFO << " rtmp sent audio_header now : " << base::TTime::NowMS() << " host : " << user_id_;
            
            // 重置音频头
            audio_header_.reset();
        }
    }    
    // 如果没有元数据和音频头，但有视频头
    else if (video_header_)
    {
        // 推送视频头帧，标头参数为 true
        auto ret = PushFrame(video_header_, true);

        // 如果推送成功
        if (ret)
        {
            // 记录日志
            LIVE_INFO << " rtmp sent video_header now : " << base::TTime::NowMS() << " host : " << user_id_;
            
            // 重置视频头
            video_header_.reset();
        }
    }
    // 如果没有元数据、音频头和视频头，但输出帧不为空
    else if (!out_frames_.empty())  
    {
        // 推送输出帧
        auto ret = PushFrames(out_frames_);

        // 如果推送成功，清空输出帧列表
        if (ret)
        {
            out_frames_.clear();
        }
    }  
    else        // 如果都没有，调用 Deactive() 方法
    {
        Deactive();
    }

    // 返回 true，表示帧处理完成
    return true;
}

UserType RtmpPlayerUser::GetUserType() const
{
    // 返回用户类型，指定为 RTMP 播放器用户类型
    return UserType::kUserTypePlayerRtmp;
}

bool RtmpPlayerUser::PushFrame(PacketPtr &packet,bool is_header)
{
    // 获取当前连接的 RTMP 上下文
    auto cx = connection_->GetContext<RtmpContext>(kRtmpContext);

    // 检查上下文是否有效且已准备好
    if (!cx || !cx->Ready())
    {
        // 如果无效或未准备好，返回 false
        return false;
    }

    // 初始化时间戳
    int64_t ts = 0;

    // 如果不是标头帧，进行时间戳校正
    if (!is_header)
    {
        ts = time_corrector_.CorrectTimestamp(packet);
    }

    // 构建 RTMP 数据块
    cx->BuildChunk(packet, ts, is_header);

    // 发送数据块
    cx->Send();

    // 返回 true，表示帧成功推送
    return true;
}

bool RtmpPlayerUser::PushFrames(std::vector<PacketPtr> &list)
{
    // 获取当前连接的 RTMP 上下文
    auto cx = connection_->GetContext<RtmpContext>(kRtmpContext);

    // 检查上下文是否有效且已准备好
    if (!cx || !cx->Ready())
    {
        // 如果无效或未准备好，返回 false
        return false;
    }

    // 初始化时间戳
    int64_t ts = 0;

    // 遍历帧列表
    for(int i = 0; i < list.size(); i++)
    {
        // 获取当前帧
        PacketPtr &packet = list[i];

        // 校正时间戳
        ts = time_corrector_.CorrectTimestamp(packet);

        // 构建 RTMP 数据块
        cx->BuildChunk(packet, ts);
    }
    
    // 发送所有构建的数据块
    cx->Send();

    // 返回 true，表示帧成功推送
    return true;
}