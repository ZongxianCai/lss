#include "CodecUtils.h"

using namespace lss::live;

bool CodecUtils::IsCodecHeader(const PacketPtr &packet)
{
    // 如果包大小大于1字节
    if (packet->PacketSize() > 1)
    {
        // 获取数据指针，从第二个字节开始
        const char *b = packet->Data() + 1;

        // 如果当前字节为0
        if (*b == 0)
        {
            // 是编解码头，返回 true
            return true;
        }
    }

    // 不是编解码头，返回 false
    return false;
}

bool CodecUtils::IsKeyFrame(const PacketPtr &packet)
{
    // 如果包大小大于0字节
    if (packet->PacketSize() > 0)
    {
        // 获取数据指针
        const char *b = packet->Data();
        // 检查首个字节的高四位是否为1，判断是否为关键帧
        return ((*b >> 4) & 0x0f) == 1;
    }
    // 包大小为0，返回 false
    return false;
}