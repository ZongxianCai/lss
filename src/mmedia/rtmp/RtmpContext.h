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
            // ------------------------------- 数据接收部分 -------------------------------
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

            // ------------------------------- 数据发送部分 -------------------------------
            // 构建一个 RTMP 块（chunk），将数据封装成 RTMP 协议格式
            bool BuildChunk(const PacketPtr &packet, uint32_t timestamp = 0, bool fmt0 = false);

            // 发送数据，将构建好的块发送出去
            void Send();

            // 判断当前是否准备好发送数据
            bool Ready() const;

            // 析构函数，使用默认析构行为
            ~RtmpContext() = default;

        private:
            // ------------------------------- 数据发送部分 -------------------------------
            // 构建一个 RTMP 块（chunk），使用右值引用的方式接收数据包
            bool BuildChunk (PacketPtr &&packet, uint32_t timestamp = 0, bool fmt0 = false);

            // 检查并发送数据，确保满足发送条件后执行发送
            void CheckAndSend();

            // 将数据包推入发送队列中，等待发送
            void PushOutQueue(PacketPtr &&packet);

            // ------------------------------- 数据接收部分 -------------------------------
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

            // ------------------------------- 数据发送部分 -------------------------------
            // 用于临时存储发送的数据块的缓冲区，大小为 4096 字节
            char out_buffer_[4096];

            // 指向缓冲区当前写入位置的指针
            char *out_current_{nullptr};

            // 用于存储不同 Chunk Stream ID (CSID) 的时间戳增量
            std::unordered_map<uint32_t, uint32_t> out_deltas_;

            // 用于存储不同 CSID 的消息头部信息
            std::unordered_map<uint32_t, RtmpMsgHeaderPtr> out_message_headers_;

            // 发送时使用的块大小，默认为 4096 字节
            int32_t out_chunk_size_{4096};

            // 用于存储等待发送的数据包队列
            std::list<PacketPtr> out_waiting_queue_;

            // 用于存储正在发送的数据块列表
            std::list<BufferNodePtr> sending_bufs_;

            // 用于存储正在发送的数据包队列
            std::list<PacketPtr> out_sending_packets_;

            // 表示当前是否正在发送数据的标志位
            bool sending_{false};
        };

        // 定义智能指针类型 RtmpContextPtr，用于管理 RtmpContext 对象的生命周期
        using RtmpContextPtr = std::shared_ptr<RtmpContext>;
    }
}