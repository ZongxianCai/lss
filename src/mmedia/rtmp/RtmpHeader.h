#pragma once 
#include <cstdint>
#include <memory>

namespace lss
{
    namespace mm
    {
        // 定义 RTMP 消息类型的枚举
        enum RtmpMsgType 
        {
            kRtmpMsgTypeChunkSize = 1,  // 块大小消息类型
            kRtmpMsgTypeBytesRead = 3,  // 读取字节数消息类型
            kRtmpMsgTypeUserControl,    // 用户控制消息类型
            kRtmpMsgTypeWindowACKSize,  // 窗口确认大小消息类型
            kRtmpMsgTypeSetPeerBW,      // 设置对等带宽消息类型
            kRtmpMsgTypeAudio = 8,      // 音频消息类型
            kRtmpMsgTypeVideo,          // 视频消息类型
            kRtmpMsgTypeAMF3Meta = 15,  // AMF3元数据消息类型
            kRtmpMsgTypeAMF3Shared,     // AMF3共享对象消息类型
            kRtmpMsgTypeAMF3Message,    // AMF3消息类型
            kRtmpMsgTypeAMFMeta,        // AMF元数据消息类型
            kRtmpMsgTypeAMFShared,      // AMF共享对象消息类型
            kRtmpMsgTypeAMFMessage,     // AMF消息类型
            kRtmpMsgTypeMetadata = 22   // 元数据消息类型
        };

        enum RtmpFmt
        {
            kRtmpFmt0 = 0,              // RTMP格式0
            kRtmpFmt1,                  // RTMP格式1
            kRtmpFmt2,                  // RTMP格式2
            kRtmpFmt3                   // RTMP格式3
        };

        enum RtmpCSID
        {
            kRtmpCSIDCommand = 2,       // 命令消息的通道ID
            kRtmpCSIDAMFIni = 3,        // AMF初始化消息的通道ID
            kRtmpCSIDAudio = 4,         // 音频消息的通道ID
            kRtmpCSIDAMF = 5,           // AMF消息的通道ID
            kRtmpCSIDVideo = 6          // 视频消息的通道ID
        };

        #define kRtmpMsID0 0            // 定义消息流ID 0
        #define kRtmpMsID1 1            // 定义消息流ID 1

        #pragma pack(push)              // 保存当前的内存对齐
        #pragma pack(1)                 // 设置内存对齐为1字节

            // 定义RTMP消息头结构
            struct RtmpMsgHeader
            {
                uint32_t cs_id{0};      // 标识消息所属的通道
                uint32_t timestamp{0};  // 时间戳（增量）
                uint32_t msg_len{0};    // 消息长度
                uint8_t  msg_type{0};   // 消息类型ID
                uint32_t msg_sid{0};    // 消息流ID

                // 构造函数
                RtmpMsgHeader()
                    : cs_id(0)
                    , timestamp(0)
                    , msg_len(0)
                    , msg_type(0)
                    , msg_sid(0)
                {

                }
            };

        // 恢复内存对齐到默认状态
        #pragma pack()

        // 定义RtmpMsgHeader的智能指针类型
        using RtmpMsgHeaderPtr = std::shared_ptr<RtmpMsgHeader>;
    }
}