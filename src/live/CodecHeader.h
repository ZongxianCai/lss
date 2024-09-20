#pragma once
#include "mmedia/base/Packet.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace lss
{
    namespace live
    {
        using namespace lss::mm;

        // CodecHeader类，处理音视频编解码相关的头信息
        class CodecHeader
        {
        public:
            // 构造函数，初始化CodecHeader对象
            CodecHeader();

            // 获取第idx个元数据包
            PacketPtr  Meta(int idx);

            // 获取第idx个视频头信息包
            PacketPtr  VideoHeader(int idx);

            // 获取第idx个音频头信息包
            PacketPtr  AudioHeader(int idx);

            // 保存元数据包
            void SaveMeta(const PacketPtr &packet);

            // 解析元数据包
            void ParseMeta(const PacketPtr &packet);

            // 保存音频头信息包
            void SaveAudioHeader(const PacketPtr &packet);

            // 保存视频头信息包
            void SaveVideoHeader(const PacketPtr &packet);

            // 解析编解码头信息，返回解析是否成功
            bool ParseCodecHeader(const PacketPtr &packet);

            // 析构函数，清理资源
            ~CodecHeader();

        private:
            // 存储视频头信息的包
            PacketPtr video_header_;

            // 存储音频头信息的包
            PacketPtr audio_header_;

            // 存储元数据包
            PacketPtr meta_;

            // 元数据版本号
            int meta_version_{0};

            // 音频头信息版本号
            int audio_version_{0};

            // 视频头信息版本号
            int video_version_{0};

            // 存储多个视频头信息包
            std::vector<PacketPtr> video_header_packets_;

            // 存储多个音频头信息包
            std::vector<PacketPtr> audio_header_packets_;

            // 存储多个元数据包
            std::vector<PacketPtr> meta_packets_;

            // 编解码开始的时间戳
            int64_t start_timestamp_{0};
        };
    }
}