#include "Stream.h"
#include "base/TTime.h"
#include "live/base/CodecUtils.h"
#include "live/base/LiveLog.h"
#include "Session.h"

using namespace lss::live;
using namespace lss::base;

Stream::Stream(Session& s, const std::string &session_name)
    : session_(s)                           // 初始化成员变量 session_ 为传入的 Session 引用
    , session_name_(session_name)           // 初始化成员变量 session_name_ 为传入的会话名称
    , packet_buffer_(packet_buffer_size_)   // 初始化数据包缓冲区大小为 packet_buffer_size_
{
    // 获取当前时间戳并赋值给 stream_time_
    stream_time_ = TTime::NowMS();

    // 获取当前时间戳并赋值给 start_timestamp_
    start_timestamp_ = TTime::NowMS();
}

int64_t Stream::ReadyTime() const 
{
    // 返回 ready_time_ 的值
    return ready_time_;
}

int64_t Stream::SinceStart() const 
{
    // 返回当前时间减去开始时间戳
    return TTime::NowMS() - start_timestamp_;
}

bool Stream::Timeout()
{
    // 计算当前时间与流时间的差值
    auto delta = TTime::NowMS() - stream_time_;

    // 如果差值大于 20 秒
    if (delta > 20 * 1000)
    {
        // 返回 true，表示超时
        return true;
    }

    // 否则返回 false，表示未超时
    return false;
}

int64_t Stream::DataTime() const 
{
    // 返回 data_coming_time_ 的值
    return data_coming_time_;
}

const std::string &Stream::SessionName() const
{
    // 返回 session_name_ 的引用
    return session_name_;
}

int32_t Stream::StreamVersion() const
{
    // 返回 stream_version_ 的值
    return stream_version_;
}

bool Stream::HasMedia() const
{
    // 返回是否有音频、视频或元数据
    return has_audio_ || has_video_ || has_meta_;
}

bool Stream::Ready() const
{
    // 返回 ready_ 的值
    return ready_;
}

void Stream::SetReady(bool ready)
{
    // 将 ready_ 设置为 true，表示流已准备好
    ready_ = true;

    // 获取当前时间戳并赋值给 ready_time_
    ready_time_ = TTime::NowMS();
}

void Stream::AddPacket(PacketPtr && packet)
{
    // 校正数据包的时间戳
    auto t = time_corrector_.CorrectTimestamp(packet);

    // 设置数据包的时间戳
    packet->SetTimeStamp(t);

    {
        // 创建互斥锁，保护临界区
        std::lock_guard<std::mutex> lk(lock_);

        // 增加帧索引并获取新的索引值
        auto index = ++frame_index_;

        // 设置数据包的索引
        packet->SetIndex(index);

        // 如果是视频并且是关键帧
        if (packet->IsVideo() && CodecUtils::IsKeyFrame(packet))
        {
            // 设置流为准备状态
            SetReady(true);

            // 设置数据包类型为视频关键帧
            packet->SetPacketType(kPacketTypeVideo | kFrameTypeKeyFrame);
        }

        // 如果是编解码头
        if (CodecUtils::IsCodecHeader(packet))
        {
            // 解析编解码头
            codec_headers_.ParseCodecHeader(packet);

            // 如果是视频
            if (packet->IsVideo())
            {
                // 标记为有视频
                has_video_ = true;

                // 增加流版本
                stream_version_++;
            }
            // 如果是音频
            else if (packet->IsAudio())
            {
                // 标记为有音频
                has_audio_ = true;

                // 增加流版本
                stream_version_++;
            }
            // 如果是元数据
            else if (packet->IsMeta())
            {
                // 标记为有元数据
                has_meta_ = true;

                // 增加流版本
                stream_version_++;
            }
        }

        // 将帧添加到 GOP 管理器
        gop_mgr_.AddFrame(packet);

        // 将数据包移动到缓冲区
        packet_buffer_[index % packet_buffer_size_] = std::move(packet);

        // 计算最小索引
        auto min_idx = frame_index_ - packet_buffer_size_;

        // 如果最小索引大于 0
        if (min_idx > 0)
        {
            // 清除过期的 GOP
            gop_mgr_.ClearExpriedGop(min_idx);
        }
    }

    // 如果数据到达时间为 0
    if (data_coming_time_ == 0)
    {
        // 获取当前时间并赋值给 data_coming_time_
        data_coming_time_ = TTime::NowMS();
    }

    // 获取当前时间并赋值给 stream_time_
    stream_time_ = TTime::NowMS();

    // 加载当前帧索引
    auto frame = frame_index_.load();

    // 如果帧索引小于 300 或每 5 帧一次
    if (frame < 300 || frame % 5 == 0)
    {
        // 激活所有播放
        session_.ActiveAllPlayers();
    }
}

void Stream::GetFrames(const PlayerUserPtr &user)
{
    // 如果没有媒体
    if (!HasMedia())
    {
        // 直接返回
        return;
    }

    // 如果用户有元数据、音频头、视频头或输出帧不为空
    if (user->meta_ || user->audio_header_ || user->video_header_ || !user->out_frames_.empty())
    {
        // 直接返回
        return;
    }

    // 创建互斥锁，保护临界区
    std::lock_guard<std::mutex> lk(lock_);

    // 如果用户的输出索引有效
    if (user->out_index_ >= 0)
    {
        // 计算最小索引
        int min_idx = frame_index_ - packet_buffer_size_;

        // 获取用户应用的信息中的内容延迟
        int content_lantency = user->GetAppInfo()->content_latency;

        // 如果用户的输出索引小于最小索引，或者当前时间戳与用户输出帧时间戳的差值大于两倍的内容延迟
        if ((user->out_index_ < min_idx) || ((gop_mgr_.LastestTimeStamp() - user->out_frame_timestamp_) > 2 * content_lantency))
        {
            LIVE_INFO << " need skip out index : " << user->out_index_
                    << " , min idx : " << min_idx
                    << " , out timestamp : " << user->out_frame_timestamp_
                    << " , latest timestamp : " << gop_mgr_.LastestTimeStamp();

            // 跳过当前帧
            SkipFrame(user);
        }    
    }
    else    // 如果用户的输出索引无效
    {
        // 定位 GOP，如果失败，直接返回
        if (!LocateGop(user))
        {
            return;
        }
    }

    // 获取下一帧
    GetNextFrame(user);
}

bool Stream::LocateGop(const PlayerUserPtr &user)
{
    // 获取用户应用的信息中的内容延迟
    int content_lantency = user->GetAppInfo()->content_latency;

    // 初始化延迟变量
    int lantency = 0;

    // 根据内容延迟获取 GOP 索引
    auto idx = gop_mgr_.GetGopByLatency(content_lantency, lantency);

    // 如果找到有效的 GOP 索引
    if (idx != -1)
    {
        // 设置用户的输出索引为找到的索引减一
        user->out_index_ = idx - 1;
    }
    else    // 如果未找到有效的 GOP 索引
    {
        // 获取用户已消耗的时间
        auto elapsed = user->ElapsedTime();

        // 如果消耗时间大于等于 1000 毫秒且未超时
        if (elapsed >= 1000 && !user->wait_timeout_)
        {
            // 记录超时日志
            LIVE_DEBUG << " wait Gop keyframe timeout. host : " << user->user_id_;
            // 设置用户等待超时标志
            user->wait_timeout_ = true;
        }
        // 返回 false
        return false;
    }

    // 检查用户是否仍在等待元数据
    user->wait_meta_ = (user->wait_meta_ && has_meta_);

    // 如果用户在等待元数据
    if (user->wait_meta_)
    {
        // 获取指定索引的元数据
        auto meta = codec_headers_.Meta(idx);

        // 如果找到了元数据
        if (meta)
        {
            // 设置不再等待元数据
            user->wait_meta_ = false;
            // 保存元数据
            user->meta_ = meta;
            // 设置元数据索引
            user->meta_index_ = meta->Index();
        }
    }

    // 检查用户是否仍在等待音频头
    user->wait_audio_ = (user->wait_audio_ && has_audio_);

    // 如果用户在等待音频头
    if (user->wait_audio_)
    {
        // 获取指定索引的音频头
        auto audio_header = codec_headers_.AudioHeader(idx);

        // 如果找到了音频头
        if (audio_header)
        {
            // 设置不再等待音频头
            user->wait_audio_= false;
            // 保存音频头
            user->audio_header_ = audio_header;
            // 设置音频头索引
            user->audio_header_index_ = audio_header->Index();
        }
    }

    // 检查用户是否仍在等待视频头
    user->wait_video_ = (user->wait_video_ && has_video_);
    
    // 如果用户在等待视频头
    if (user->wait_video_)
    {
        // 获取指定索引的视频头
        auto video_header = codec_headers_.VideoHeader(idx);

        // 如果找到了视频头
        if (video_header)
        {
            // 设置不再等待视频头
            user->wait_video_ = false;
            // 保存视频头
            user->video_header_ = video_header;
            // 设置视频头索引
            user->video_header_index_ = video_header->Index();
        }
    }     

    // 如果用户仍在等待元数据、音频头、视频头，或未找到有效的索引
    if (user->wait_meta_ || user->wait_audio_ || user->wait_video_ || idx == -1)
    {
        // 获取用户已消耗的时间
        auto elapsed = user->ElapsedTime();

        // 如果消耗时间大于等于 1000 毫秒且未超时
        if (elapsed >= 1000 && !user->wait_timeout_)
        {
            // 记录超时日志
            LIVE_DEBUG << " wait Gop keyframe timeout elapsed : " << elapsed 
                    << " ms, frame index : " << frame_index_
                    << " , gop size : " << gop_mgr_.GopSize()
                    <<" . host : " << user->user_id_;
            
            // 设置用户等待超时标志
            user->wait_timeout_ = true;
        }

        // 返回 false
        return false;
    }   

    // 如果到此，表示成功定位到 GOP
    user->wait_meta_ = true;                // 设置用户等待元数据标志
    user->wait_audio_ = true;               // 设置用户等待音频头标志
    user->wait_video_ = true;               // 设置用户等待视频头标志
    user->out_version_ = stream_version_;   // 设置用户的输出版本为当前流版本

    // 获取用户已消耗的时间
    auto elapsed = user->ElapsedTime();

    // 记录成功定位 GOP 的日志
    LIVE_DEBUG << " locate GOP sucess, elapsed : " << elapsed 
                << " ms, gop idx : " << idx
                << " , frame index : " << frame_index_
                << " , lantency : " << lantency
                << " , user : " << user->user_id_;

    // 返回 true，表示成功
    return true;
}

void Stream::SkipFrame(const PlayerUserPtr &user)
{
    // 获取用户应用的信息中的内容延迟
    int content_lantency = user->GetAppInfo()->content_latency;

    // 初始化延迟变量
    int lantency = 0;

    // 根据内容延迟获取 GOP 索引
    auto idx = gop_mgr_.GetGopByLatency(content_lantency, lantency);

    // 如果未找到有效的 GOP 索引或索引小于等于用户的输出索引
    if (idx == -1 || idx <= user->out_index_)
    {
        // 直接返回
        return;
    }

    // 获取指定索引的元数据
    auto meta = codec_headers_.Meta(idx);

    // 如果找到了元数据
    if (meta)
    {
        // 如果元数据索引大于用户的元数据索引
        if (meta->Index() > user->meta_index_)
        {
            // 更新用户的元数据
            user->meta_ = meta;

            // 更新用户的元数据索引
            user->meta_index_ = meta->Index();
        }
    }

    // 获取指定索引的音频头
    auto audio_header = codec_headers_.AudioHeader(idx);
    
    // 如果找到了音频头
    if (audio_header)
    {
        // 如果音频头索引大于用户的音频头索引
        if(audio_header->Index() > user->audio_header_index_)
        {
            // 更新用户的音频头
            user->audio_header_ = audio_header;

            // 更新用户的音频头索引
            user->audio_header_index_ = audio_header->Index();
        }
    }

    // 获取指定索引的视频头
    auto video_header = codec_headers_.VideoHeader(idx);

    // 如果找到了视频头
    if (video_header)
    {
        // 如果视频头索引大于用户的视频头索引
        if (video_header->Index() > user->video_header_index_)
        {
            // 更新用户的视频头
            user->video_header_ = video_header;

            // 更新用户的视频头索引
            user->video_header_index_ = video_header->Index();
        }
    }      

    // 记录跳过帧的日志
    LIVE_DEBUG << " skip frame " << user->out_index_ << " -> " << idx
                << " , lantency : " << lantency 
                << " , frame_index : " << frame_index_
                << " , host : " << user->user_id_;

    // 更新用户的输出索引为当前索引减一
    user->out_index_ = idx - 1;  
}

void Stream::GetNextFrame(const PlayerUserPtr &user)
{
    // 从用户的输出索引加一开始
    auto idx = user->out_index_ + 1;

    // 获取当前最大帧索引
    auto max_idx = frame_index_.load();

    // 最多获取 10 帧
    for (int i = 0; i < 10; i++)
    {
        // 如果索引超出最大帧索引
        if (idx > max_idx)
        {
            // 结束循环
            break;
        }

        // 获取对应索引的数据包
        auto &pkt = packet_buffer_[idx % packet_buffer_size_];

        // 如果数据包存在
        if (pkt)
        {
            // 将数据包添加到用户的输出帧列表
            user->out_frames_.emplace_back(pkt);

            // 更新用户的输出索引为当前数据包的索引
            user->out_index_ = pkt->Index();

            // 更新用户的输出帧时间戳为当前数据包的时间戳
            user->out_frame_timestamp_ = pkt->TimeStamp();

            // 更新索引为当前数据包的索引加一，以便获取下一帧
            idx = pkt->Index() + 1;
        }
        else            // 如果数据包不存在
        {
            // 结束循环
            break;
        }
    }
}