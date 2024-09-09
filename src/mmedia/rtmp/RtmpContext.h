#pragma once
#include <cstdint>
#include <unordered_map>
#include "network/net/TcpConnection.h"
#include "RtmpHandShake.h"
#include "RtmpHandler.h"
#include "RtmpHeader.h"
#include "mmedia/base/Packet.h"
#include "mmedia/rtmp/amf/AMFObject.h"

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

        // ------------------------------- Rtmp协议控制消息和用户控制消息 -------------------------------
        // 定义了 RTMP 协议中与用户控制消息相关的事件类型
        enum RtmpEventType
        {
            kRtmpEventTypeStreamBegin = 0,  // 表示流开始事件。当客户端或服务器开始发送音视频流时，会触发此事件
            kRtmpEventTypeStreamEOF,        // 表示流结束事件。当音视频流传输结束时，会触发此事件
            kRtmpEventTypeStreamDry,        // 表示流干涸事件。意味着当前流没有更多数据可发送，可能是由于网络问题或其他原因导致的流中断
            kRtmpEventTypeSetBufferLength,  // 设置缓冲区长度事件。用于调整客户端的缓冲区大小，避免因缓冲区溢出或不足而导致的流播放问题
            kRtmpEventTypeStreamsRecorded,  // 流录制事件。表示流已经被记录下来，通常用于点播或录像的场景
            kRtmpEventTypePingRequest,      // Ping 请求事件。用于检测连接的延迟或确认连接的有效性，类似于网络中的 ICMP Ping
            kRtmpEventTypePingResponse      // Ping 响应事件。对 Ping 请求的响应，确认连接状态
        };

        // 定义了 RTMP 协议中与用户控制消息相关的命令回调的别名
        using CommandFunc = std::function<void (AMFObject &obj)>;

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

            // 拉流函数，用于拉取指定的流
            void Play(const std::string &url);

            // 推流函数，用于发布指定的流
            void Publish(const std::string &url);

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

            // ------------------------------- Rtmp协议控制消息和用户控制消息 -------------------------------
            // 处理RTMP协议中的“Chunk Size”消息
            void HandleChunkSize(PacketPtr &packet);

            // 处理RTMP协议中的“Window Size”消息
            void HandleAckWindowSize(PacketPtr &packet);

            // 处理RTMP协议中的用户控制消息
            void HandleUserMessage(PacketPtr &packet);

            /* 处理 AMF 命令
             * param data: 包含 AMF 消息的智能指针
             * param amf3: 布尔值，指示是否为 AMF3 格式（默认为 false，即 AMF0 格式）
             */
            void HandleAmfCommand(PacketPtr &data, bool amf3 = false);

            // 发送设置 Chunk Size 的消息
            void SendSetChunkSize();

            // 发送设置确认窗口大小的消息
            void SendAckWindowSize();

            // 发送设置对端带宽的消息
            void SendSetPeerBandwidth();

            // 发送已接收字节数的消息
            void SendBytesRecv();

            /*
             * 发送用户控制消息
             * nType: 控制消息类型
             * value1: 消息的第一个值
             * value2: 消息的第二个值（可能为空）
             */
            void SendUserCtrlMessage(short nType, uint32_t value1, uint32_t value2);

            // ------------------------------ 命令解析与实现部分 ------------------------------

            // 发送 RTMP 连接请求
            void SendConnect();

            // 处理 RTMP 连接响应
            void HandleConnect(AMFObject &obj);

            // 发送 RTMP 创建流请求
            void SendCreateStream();

            // 处理 RTMP 创建流响应
            void HandleCreateStream(AMFObject &obj);

            // 发送 RTMP 状态消息
            void SendStatus(const std::string &level, const std::string &code, const std::string &description);
            
            // 发送 RTMP 播放请求
            void SendPlay();

            // 处理 RTMP 播放响应
            void HandlePlay(AMFObject &obj);

            // 解析 RTMP URL 中的流名称和 tcUrl
            void ParseNameAndTcUrl();

            // 发送 RTMP 发布流请求
            void SendPublish();

            // 处理 RTMP 发布流响应
            void HandlePublish(AMFObject &obj);

            // 处理 RTMP 调用结果响应
            void HandleResult(AMFObject &obj);

            // 处理 RTMP 错误消息
            void HandleError(AMFObject &obj);

            void SetPacketType(PacketPtr &packet);

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

            // ------------------------------- Rtmp协议控制消息和用户控制消息 -------------------------------
            // 确认窗口大小，单位是字节，默认值为2500000字节（约2.5MB）
            int32_t ack_size_{2500000};

            // 已接收的字节数，初始化为0
            int32_t in_bytes_{0};

            // 上一个计算的剩余字节数，初始化为0
            int32_t last_left_{0};

            // ------------------------------ 命令解析与实现部分 ------------------------------
            // RTMP 连接中用于指定目标应用
            std::string app_;

            // RTMP 连接中用于指定流的地址
            std::string tc_url_;

            // 流的名称，用于指定要播放或发布的流
            std::string name_;

            // 会话名称，用于标识当前 RTMP 会话
            std::string session_name_;

            // 附加参数，用于传递额外的连接信息
            std::string param_;
            
            // 标识当前 RTMP 连接是否作为播放器（false 表示发布者，true 表示播放器）
            bool is_player_{false};

            // 存储所有命令
            std::unordered_map<std::string, CommandFunc> commands_;

            // 判断是否为客户端
            bool is_client_{false};
        };

        // 定义智能指针类型 RtmpContextPtr，用于管理 RtmpContext 对象的生命周期
        using RtmpContextPtr = std::shared_ptr<RtmpContext>;
    }
}