#pragma once

#include "mmedia/base/Packet.h"

namespace lss
{
    namespace live
    {
        using namespace lss::mm;

        class CodecUtils
        {
        public:
            // 静态方法：检查包是否是编解码头
            static bool IsCodecHeader(const PacketPtr &packet);

            // 静态方法：检查包是否是关键帧
            static bool IsKeyFrame(const PacketPtr &packet);
        };
    }
}