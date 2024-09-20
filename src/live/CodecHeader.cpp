#include <sstream>
#include "CodecHeader.h"
#include "base/TTime.h"
#include "live/base/LiveLog.h"
#include "mmedia/rtmp/amf/AMFObject.h"

using namespace lss::live;
using namespace lss::mm;

CodecHeader::CodecHeader()
{
    // 构造函数，将当前的毫秒级时间戳赋值给start_timestamp_
    start_timestamp_ = lss::base::TTime::NowMS();
}

PacketPtr CodecHeader::Meta(int idx)
{
    // 如果传入的索引小于等于0，直接返回当前保存的meta_元数据包
    if (idx <= 0)
    {
        return meta_;
    }

    // 反向迭代meta_packets_，从最后一个元素开始
    auto iter = meta_packets_.rbegin();

    // 遍历meta_packets_，找到索引小于等于idx的包
    for (; iter != meta_packets_.rend(); iter++)
    {
        // 获取当前包
        PacketPtr pkt = *iter;

        // 如果当前包的索引小于等于传入的idx，则返回该包
        if (pkt->Index() <= idx)
        {
            return pkt;
        }
    }

    // 如果遍历完所有包都没有找到，返回meta_
    return meta_;
}

PacketPtr CodecHeader::VideoHeader(int idx)
{
    // 如果传入的索引小于等于0，直接返回当前保存的视频头信息包
    if (idx <= 0)
    {
        return video_header_;
    }

    // 反向迭代video_header_packets_，从最后一个元素开始
    auto iter = video_header_packets_.rbegin();

    // 遍历video_header_packets_，找到索引小于等于idx的包
    for (; iter != video_header_packets_.rend(); iter++)
    {
        // 获取当前包
        PacketPtr pkt = *iter;

        // 如果当前包的索引小于等于传入的idx，则返回该包
        if (pkt->Index() <= idx)
        {
            return pkt;
        }
    }

    // 如果遍历完所有包都没有找到，返回video_header_
    return video_header_;
}

PacketPtr CodecHeader::AudioHeader(int idx)
{
    // 如果传入的索引小于等于0，直接返回当前保存的音频头信息包
    if (idx <= 0)
    {
        return audio_header_;
    }

    // 反向迭代audio_header_packets_，从最后一个元素开始
    auto iter = audio_header_packets_.rbegin();

    // 遍历audio_header_packets_，找到索引小于等于idx的包
    for (; iter != audio_header_packets_.rend(); iter++)
    {
        // 获取当前包
        PacketPtr pkt = *iter;

        // 如果当前包的索引小于等于传入的idx，则返回该包
        if (pkt->Index() <= idx)
        {
            return pkt;
        }
    }

    // 如果遍历完所有包都没有找到，返回audio_header_
    return audio_header_;
}

void CodecHeader::SaveMeta(const PacketPtr &packet)
{
    // 将传入的元数据包保存到成员变量meta_
    meta_ = packet;

    // 增加元数据版本号
    ++ meta_version_;

    // 将元数据包添加到meta_packets_列表中
    meta_packets_.emplace_back(packet);

    // 输出保存元数据的日志信息
    LIVE_TRACE << " save meta, meta version : " << meta_version_
                << " , size : " << packet->PacketSize()
                << " , elapse : " << TTime::NowMS() - start_timestamp_ << " ms\n";
}

void CodecHeader::ParseMeta(const PacketPtr &packet)
{
    // 创建AMFObject实例，用于解析AMF格式数据
    AMFObject obj;

    // 尝试解码传入的包，如果解码失败，则直接返回
    if (!obj.Decode(packet->Data(), packet->PacketSize()))
    {
        return;
    }

    // 创建一个字符串流，用于构建日志信息
    std::stringstream ss;

    // 记录解析元数据的日志起始信息
    ss << "ParseMeta ";

    // 解析宽度信息
    AMFAnyPtr widthPtr = obj.Property("width");
    if (widthPtr)
    {
        ss << " , width : " << (uint32_t)widthPtr->Number();
    }
    
    // 解析高度信息
    AMFAnyPtr heightPtr = obj.Property("height");
    if (heightPtr)
    {
        ss << " , height : " << (uint32_t)heightPtr->Number();
    }

    // 解析视频编码ID
    AMFAnyPtr videocodecidPtr = obj.Property("videocodecid");
    if (videocodecidPtr)
    {
        ss << " , videocodecid : " << (uint32_t)videocodecidPtr->Number();
    }

    // 解析帧率信息
    AMFAnyPtr frameratePtr = obj.Property("framerate");
    if (frameratePtr)
    {
        ss << " , framerate : " << (uint32_t)frameratePtr->Number();
    }

    // 解析视频数据率
    AMFAnyPtr videodataratePtr = obj.Property("videodatarate");
    if (videodataratePtr)
    {
        ss << " , videodatarate : " << (uint32_t)videodataratePtr->Number();
    }     

    // 解析音频采样率
    AMFAnyPtr audiosampleratePtr = obj.Property("audiosamplerate");
    if(audiosampleratePtr)
    {
        ss << " , audiosamplerate : " << (uint32_t)audiosampleratePtr->Number();
    }

    // 解析音频样本大小
    AMFAnyPtr audiosamplesizePtr = obj.Property("audiosamplesize");
    if (audiosamplesizePtr)
    {
        ss << " , audiosamplesize : " << (uint32_t)audiosamplesizePtr->Number();
    }

    // 解析音频编码ID
    AMFAnyPtr audiocodecidPtr = obj.Property("audiocodecid");
    if (audiocodecidPtr)
    {
        ss << " , audiocodecid : " << (uint32_t)audiocodecidPtr->Number();
    }

    // 解析音频数据率
    AMFAnyPtr audiodataratePtr = obj.Property("audiodatarate");
    if (audiodataratePtr)
    {
        ss << " , audiodatarate : " << (uint32_t)audiodataratePtr->Number();
    }

    // 解析时长信息
    AMFAnyPtr durationPtr = obj.Property("duration");
    if (durationPtr)
    {
        ss << " , duration : " << (uint32_t)durationPtr->Number();
    }

    // 解析编码器信息
    AMFAnyPtr encoderPtr = obj.Property("encoder");
    if (encoderPtr)
    {
        ss << " , encoder : " << encoderPtr->String();
    }

    // 解析服务器信息
    AMFAnyPtr serverPtr = obj.Property("server");
    if (serverPtr)
    {
        ss << " , server : " << serverPtr->String();
    }     

    // 输出解析结果到日志
    LIVE_TRACE << ss.str();                     
}

void CodecHeader::SaveAudioHeader(const PacketPtr &packet)
{
    // 将传入的音频头信息包保存到成员变量audio_header_
    audio_header_ = packet;

    // 增加音频头信息的版本号
    ++ audio_version_;

    // 将音频头信息包添加到audio_header_packets_列表中
    audio_header_packets_.emplace_back(packet);

    // 输出保存音频头信息的日志信息
    LIVE_TRACE << " save audio header, version : " << audio_version_
                << " , size : " << packet->PacketSize()
                << " , elapse : " << TTime::NowMS() - start_timestamp_ << " ms\n";
}

void CodecHeader::SaveVideoHeader(const PacketPtr &packet)
{
    // 将传入的视频头信息包保存到成员变量video_header_
    video_header_ = packet;

    // 增加视频头信息的版本号
    ++ video_version_;

    // 将视频头信息包添加到video_header_packets_列表中
    video_header_packets_.emplace_back(packet);

    // 输出保存视频头信息的日志信息
    LIVE_TRACE << " save video header, version : " << video_version_
                << " , size : " << packet->PacketSize()
                << " , elapse : " << TTime::NowMS()-start_timestamp_ << " ms\n";
}

bool CodecHeader::ParseCodecHeader(const PacketPtr &packet)
{
    // 如果传入的包是元数据包，调用SaveMeta和ParseMeta方法处理元数据
    if (packet->IsMeta())
    {
        // 保存元数据包
        SaveMeta(packet);

        // 解析元数据包
        ParseMeta(packet);
    }
    // 如果传入的包是音频包，调用SaveAudioHeader方法处理音频头信息
    else if (packet->IsAudio())
    {
        // 保存音频头信息包
        SaveAudioHeader(packet);
    }
    // 如果传入的包是视频包，调用SaveVideoHeader方法处理视频头信息
    else if (packet->IsVideo())
    {
        // 保存视频头信息包
        SaveVideoHeader(packet);
    }

    // 返回true，表示解析成功
    return true;
}

// 析构函数
CodecHeader::~CodecHeader()
{

}