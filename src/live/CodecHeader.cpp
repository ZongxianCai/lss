#include "CodecHeader.h"
#include "base/TTime.h"
#include "live/base/LiveLog.h"
#include "mmedia/rtmp/amf/AMFObject.h"
#include <sstream>

using namespace lss::live;
using namespace lss::mm;

CodecHeader::CodecHeader()
{
    start_timestamp_ = lss::base::TTime::NowMS();
}


PacketPtr  CodecHeader::Meta(int idx)
{
    if(idx <= 0)
    {
        return meta_;
    }
    auto iter = meta_packets_.rbegin();
    for(;iter!=meta_packets_.rend();iter++)
    {
        PacketPtr pkt = *iter;
        if(pkt->Index() <= idx)
        {
            return pkt;
        }
    }
    return meta_;
}
PacketPtr  CodecHeader::VideoHeader(int idx)
{
    if(idx <= 0)
    {
        return video_header_;
    }
    auto iter = video_header_packets_.rbegin();
    for(;iter!=video_header_packets_.rend();iter++)
    {
        PacketPtr pkt = *iter;
        if(pkt->Index() <= idx)
        {
            return pkt;
        }
    }
    return video_header_;
}
PacketPtr  CodecHeader::AudioHeader(int idx)
{
    if(idx <= 0)
    {
        return audio_header_;
    }
    auto iter = audio_header_packets_.rbegin();
    for(;iter!=audio_header_packets_.rend();iter++)
    {
        PacketPtr pkt = *iter;
        if(pkt->Index() <= idx)
        {
            return pkt;
        }
    }
    return audio_header_;
}
void CodecHeader::SaveMeta(const PacketPtr &packet)
{
    meta_ = packet;
    ++ meta_version_;

    meta_packets_.emplace_back(packet);

    LIVE_TRACE << "save meta ,meta version:" << meta_version_
                << ",size:" << packet->PacketSize()
                << " ,elapse:" << TTime::NowMS()-start_timestamp_ << "ms\n";
}
void CodecHeader::ParseMeta(const PacketPtr &packet)
{
    AMFObject obj;
    if(!obj.Decode(packet->Data(),packet->PacketSize()))
    {
        return;
    }
    std::stringstream ss;
    ss << "ParseMeta ";

    AMFAnyPtr widthPtr = obj.Property("width");
    if(widthPtr)
    {
        ss << " ,width:" << (uint32_t)widthPtr->Number();
    }
    AMFAnyPtr heightPtr = obj.Property("height");
    if(heightPtr)
    {
        ss << " ,height:" << (uint32_t)heightPtr->Number();
    }    
    AMFAnyPtr videocodecidPtr = obj.Property("videocodecid");
    if(videocodecidPtr)
    {
        ss << " ,videocodecid:" << (uint32_t)videocodecidPtr->Number();
    }      
    AMFAnyPtr frameratePtr = obj.Property("framerate");
    if(frameratePtr)
    {
        ss << " ,framerate:" << (uint32_t)frameratePtr->Number();
    }  
    AMFAnyPtr videodataratePtr = obj.Property("videodatarate");
    if(videodataratePtr)
    {
        ss << " ,videodatarate:" << (uint32_t)videodataratePtr->Number();
    }     

    AMFAnyPtr audiosampleratePtr = obj.Property("audiosamplerate");
    if(audiosampleratePtr)
    {
        ss << " ,audiosamplerate:" << (uint32_t)audiosampleratePtr->Number();
    }  
    AMFAnyPtr audiosamplesizePtr = obj.Property("audiosamplesize");
    if(audiosamplesizePtr)
    {
        ss << " ,audiosamplesize:" << (uint32_t)audiosamplesizePtr->Number();
    }     
    AMFAnyPtr audiocodecidPtr = obj.Property("audiocodecid");
    if(audiocodecidPtr)
    {
        ss << " ,audiocodecid:" << (uint32_t)audiocodecidPtr->Number();
    }      
    AMFAnyPtr audiodataratePtr = obj.Property("audiodatarate");
    if(audiodataratePtr)
    {
        ss << " ,audiodatarate:" << (uint32_t)audiodataratePtr->Number();
    }  
    AMFAnyPtr durationPtr = obj.Property("duration");
    if(durationPtr)
    {
        ss << " ,duration:" << (uint32_t)durationPtr->Number();
    }    
    AMFAnyPtr encoderPtr = obj.Property("encoder");
    if(encoderPtr)
    {
        ss << " ,encoder:" << encoderPtr->String();
    } 
    AMFAnyPtr serverPtr = obj.Property("server");
    if(serverPtr)
    {
        ss << " ,server:" << serverPtr->String();
    }     

    LIVE_TRACE << ss.str();                     
}
void CodecHeader::SaveAudioHeader(const PacketPtr &packet)
{
    audio_header_ = packet;
    ++ audio_version_;

    audio_header_packets_.emplace_back(packet);

    LIVE_TRACE << "save audio header ,version:" << audio_version_
                << ",size:" << packet->PacketSize()
                << " ,elapse:" << TTime::NowMS()-start_timestamp_ << "ms\n";
}
void CodecHeader::SaveVideoHeader(const PacketPtr &packet)
{
    video_header_ = packet;
    ++ video_version_;

    video_header_packets_.emplace_back(packet);

    LIVE_TRACE << "save video header ,version:" << video_version_
                << ",size:" << packet->PacketSize()
                << " ,elapse:" << TTime::NowMS()-start_timestamp_ << "ms\n";
}
bool CodecHeader::ParseCodecHeader(const PacketPtr &packet)
{
    if(packet->IsMeta())
    {
        SaveMeta(packet);
        ParseMeta(packet);
    }
    else if(packet->IsAudio())
    {
        SaveAudioHeader(packet);
    }
    else if(packet->IsVideo())
    {
        SaveVideoHeader(packet);
    }
    return true;
}

CodecHeader::~CodecHeader()
{

}