#pragma once
#include <cstdint>
#include <unordered_map>
#include "network/net/TcpConnection.h"
#include "RtmpHandShake.h"
#include "RtmpHandler.h"
#include "RtmpHeader.h"
#include "mmedia/base/Packet.h"

namespace lss
{
    namespace mm
    {
        using namespace lss::network;

        // 定义一个枚举类型 RtmpContextState，用于表示 RTMP 不同的状态阶段
        enum RtmpContextState
        {
            kRtmpHandShake = 0,     // RTMP 握手阶段
            kRtmpWatingDone = 1,    // RTMP 正在等待完成握手的阶段
            kRtmpMessage = 2        // RTMP 已经进入消息处理阶段
        }; 

        class RtmpContext
        {
        public:
            // 构造函数，接收 TCP 连接指针 conn 和处理器指针 handler，并且可选参数 client 表示是否为客户端模式
            RtmpContext(const TcpConnectionPtr &conn, RtmpHandler *handler, bool client = false);

            // 解析函数，接收一个消息缓冲区 buff，并解析其中的数据，返回 int32_t 类型的结果
            int32_t Parse(MsgBuffer &buff);

            // 写入完成的回调函数，当数据成功写入时调用
            void OnWriteComplete();

            // 开始握手的函数，启动 RTMP 握手流程
            void StartHandShake();

            // 解析消息的函数，接收消息缓冲区 buff，并解析消息数据，返回 int32_t 类型的结果
            int32_t ParseMessage(MsgBuffer &buff);

            // 消息完成的回调函数，处理已经完全接收并解析的消息数据
            void MessageComplete(PacketPtr && data);

            // 析构函数，使用默认析构行为
            ~RtmpContext() = default;

        private:
            // RtmpHandShake 对象，用于管理和处理 RTMP 握手过程
            RtmpHandShake handshake_;

            // 状态变量 state_，初始值为 kRtmpHandShake，表示当前的 RTMP 状态
            int32_t state_ {kRtmpHandShake};

            // TCP 连接指针，用于管理与客户端或服务器的连接
            TcpConnectionPtr connection_;

            // RTMP 处理器指针 rtmp_handler_，用于处理 RTMP 相关的业务逻辑
            RtmpHandler *rtmp_handler_{nullptr};

            // 无序映射表，用于存储接收到的消息头部，键为消息 ID，值为消息头指针
            std::unordered_map<uint32_t, RtmpMsgHeaderPtr> in_message_headers_;

            // 无序映射表，用于存储接收到的数据包，键为消息 ID，值为数据包指针
            std::unordered_map<uint32_t, PacketPtr> in_packets_;

            // 无序映射表，用于存储时间戳增量信息，键为消息 ID，值为增量值
            std::unordered_map<uint32_t, uint32_t> in_deltas_;

            // 无序映射表，用于标记是否使用扩展时间戳，键为消息 ID，值为布尔值
            std::unordered_map<uint32_t, bool> in_ext_;

            // 输入块大小 in_chunk_size_，初始值为 128，表示解析时的块大小
            int32_t in_chunk_size_{128};
        };

        // 定义智能指针类型 RtmpContextPtr，用于管理 RtmpContext 对象的生命周期
        using RtmpContextPtr = std::shared_ptr<RtmpContext>;
    }
}