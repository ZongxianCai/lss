#include "RtmpContext.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace lss::mm;

RtmpContext::RtmpContext(const TcpConnectionPtr &conn, RtmpHandler *handler, bool client)
    : handshake_(conn, client)      // 初始化 handshake_ 对象，传入 TCP 连接和客户端标识
    , connection_(conn)             // 初始化 connection_ 成员，保存传入的 TCP 连接
    , rtmp_handler_(handler)        // 初始化 rtmp_handler_ 成员，保存传入的 RTMP 处理器指针
{

}

int32_t RtmpContext::Parse(MsgBuffer &buff)
{
    int32_t ret = 0;

    // 检查当前状态是否为握手阶段
    if (state_ == kRtmpHandShake)
    {
        // 调用握手处理函数
        ret = handshake_.HandShake(buff);

        // 握手成功，状态切换到消息处理阶段
        if (ret == 0)
        {
            state_ = kRtmpMessage;

            // 如果缓冲区还有可读数据，继续解析
            if (buff.ReadableBytes() > 0)
            {
                // 递归调用，继续解析剩余数据
                return Parse(buff);
            }
        }
        else if (ret == -1)
        {
            // 握手失败，打印错误日志
            RTMP_ERROR << " rtmp handshake error.";
        }
        else if (ret == 2)
        {
            // 握手部分完成，切换到等待完成阶段
            state_ = kRtmpWatingDone;
        }
    }
    else if (state_ == kRtmpMessage)
    {
        // 如果当前状态是消息处理阶段，调用消息解析函数
        return ParseMessage(buff);
    }

    // 返回结果，可能是 0, -1, 或 2，取决于握手结果
    return ret;
}

void RtmpContext::OnWriteComplete()
{
    // 如果当前状态是握手阶段，调用握手写入完成处理函数
    if (state_ == kRtmpHandShake)
    {
        handshake_.WriteComplete();
    }
    // 如果当前状态是等待完成阶段，切换到消息处理阶段
    else if (state_ == kRtmpWatingDone)
    {
        state_ = kRtmpMessage;
    }
    // 消息处理阶段写入完成后的操作，不做处理
    else if (state_ == kRtmpMessage)
    {

    }
}

void RtmpContext::StartHandShake()
{
    // 调用握手开始函数，启动握手流程
    handshake_.Start();
}

int32_t RtmpContext::ParseMessage(MsgBuffer &buff)
{
    // 消息格式（FMT）
    uint8_t fmt;
    // 消息 CSID、长度、流 ID、时间戳
    uint32_t csid, msg_len = 0, msg_sid = 0, timestamp = 0;
    // 消息类型
    uint8_t msg_type = 0;
    // 缓冲区内可读字节数
    uint32_t total_bytes = buff.ReadableBytes();
    // 已解析的字节数
    int32_t parsed = 0;

    // 当缓冲区内有足够的数据时进行解析
    while (total_bytes > 1)
    {
        // 获取当前缓冲区指针
        const char *pos = buff.Peek();

        parsed = 0;

        // 解析基本头部（Basic Header）
        // 获取 FMT（格式信息），占 2 位
        fmt = (*pos >> 6) & 0x03;
        // 获取 CSID（流 ID），占 6 位
        csid = *pos & 0x3F;
        parsed++;

        // 如果 CSID 为 0，需要额外读取 1 个字节
        if (csid == 0)
        {
            // 如果数据不足，返回 1 表示需要更多数据
            if (total_bytes < 2)
            {
                return 1;
            }

            // CSID 基础值为 64
            csid = 64;
            // 从下一个字节读取 CSID 增量
            csid += *((uint8_t *)(pos + parsed));
            parsed++;
        }
        // 如果 CSID 为 1，需要额外读取 2 个字节
        else if (csid == 1)
        {
            // 如果数据不足，返回 1 表示需要更多数据
            if (total_bytes < 3)
            {
                return 1;
            }

            // CSID 基础值为 64
            csid = 64;
            // 从下一个字节读取 CSID 增量
            csid += *((uint8_t *)(pos + parsed));
            parsed++;
            // 再读取下一个字节，进一步增加 CSID 值
            csid +=  *((uint8_t *)(pos + parsed)) * 256;
            parsed ++;           
        }

        // 计算剩余数据大小
        int size = total_bytes - parsed;

        // 根据不同的 FMT 值，检查剩余数据是否足够解析
        if (size == 0 || (fmt == 0 && size < 11) || (fmt == 1 && size < 7) || (fmt == 2 && size < 3))
        {
            // 数据不足，返回 1 表示需要更多数据
            return 1;
        }

        // 初始化消息长度
        msg_len = 0;
        // 初始化流 ID
        msg_sid = 0;
        // 初始化消息类型
        msg_type = 0;
        // 初始化时间戳
        timestamp = 0;
        // 定义新的时间戳
        int32_t ts = 0;

        // 获取上一次的消息头部，使用 CSID 作为键
        RtmpMsgHeaderPtr &prev = in_message_headers_[csid];

        if (!prev)
        {
            // 如果之前没有消息头部，创建一个新的
            prev = std::make_shared<RtmpMsgHeader>();
        }

        // 根据不同的 FMT 值，解析消息头部
        if (fmt == kRtmpFmt0)   // FMT0：完整消息头部
        {
            // 读取 24 位的时间戳
            ts = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            // 重置时间戳增量
            in_deltas_[csid] = 0;
            // 当前时间戳
            timestamp = ts;
            // 读取 24 位的消息长度
            msg_len = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            // 读取 8 位的消息类型
            msg_type = BytesReader::ReadUint8T(pos + parsed);
            parsed += 1;
            // 读取 32 位的消息流 ID
            memcpy(&msg_sid, pos + parsed, 4);
            parsed += 4;
        }
        else if (fmt == kRtmpFmt1)  // FMT1：部分消息头部
        {
            // 读取 24 位的时间戳增量
            ts = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            // 保存时间戳增量
            in_deltas_[csid] = ts;
            // 计算当前时间戳
            timestamp = ts + prev->timestamp;
            // 读取 24 位的消息长度
            msg_len = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            // 读取 8 位的消息类型
            msg_type = BytesReader::ReadUint8T(pos + parsed);
            parsed += 1;
            // 使用之前的消息流 ID
            msg_sid = prev->msg_sid;
        }
        else if (fmt == kRtmpFmt2)  // FMT2：最小消息头部
        {
            // 读取 24 位的时间戳增量
            ts = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            // 保存时间戳增量
            in_deltas_[csid] = ts;
            // 计算当前时间戳
            timestamp = ts + prev->timestamp;
            // 使用之前的消息长度
            msg_len = prev->msg_len;
            // 使用之前的消息类型
            msg_type = prev->msg_type;
            // 使用之前的消息流 ID
            msg_sid = prev->msg_sid;
        }    
        else if (fmt == kRtmpFmt3)  // FMT3：无消息头部
        {
            // 使用之前的时间戳增量计算当前时间戳
            timestamp = in_deltas_[csid] + prev->timestamp;
            // 使用之前的消息长度
            msg_len = prev->msg_len;
            // 使用之前的消息类型
            msg_type = prev->msg_type;
            // 使用之前的消息流 ID
            msg_sid = prev->msg_sid;
        } 

        // 检查是否使用扩展时间戳
        bool ext = (ts == 0xFFFFFF);

        if (fmt == kRtmpFmt3)   // FMT3 使用之前保存的扩展时间戳标志
        {
            ext = in_ext_[csid];
        }

        // 更新扩展时间戳标志
        in_ext_[csid] = ext;

        // 如果使用扩展时间戳
        if (ext)
        {
            // 检查是否有足够的字节用于读取扩展时间戳
            if (total_bytes - parsed < 4)
            {
                // 数据不足，返回 1 表示需要更多数据
                return 1;
            }

            // 读取 32 位的扩展时间戳
            ts = BytesReader::ReadUint32T(pos + parsed);
            parsed += 4;

            // 如果格式不是 FMT0，则更新时间戳
            if (fmt != kRtmpFmt0)
            {
                // 更新当前时间戳
                timestamp = ts + prev->timestamp;
                // 保存当前时间戳增量
                in_deltas_[csid] = ts;
            }
        }

        // 获取当前 CSID 对应的数据包指针
        PacketPtr &packet = in_packets_[csid];

        // 如果尚未初始化数据包
        if (!packet)
        {
            // 创建一个新数据包，大小为消息长度
            packet = Packet::NewPacket(msg_len);
        }

        // 从数据包中获取消息头部的扩展信息
        RtmpMsgHeaderPtr header = packet->Ext<RtmpMsgHeader>();

        // 如果数据包中没有消息头部扩展信息
        if (!header)
        {
            // 创建新的消息头部
            header = std::make_shared<RtmpMsgHeader>();
            // 将消息头部设置为数据包的扩展信息
            packet->SetExt(header);
        }

        // 更新消息头部信息
        header->cs_id = csid;
        header->msg_len = msg_len;
        header->msg_sid = msg_sid;
        header->msg_type = msg_type;
        header->timestamp = timestamp;

        // 计算本次可读取的字节数，受限于数据包剩余空间和块大小
        int bytes = std::min(packet->Space(), in_chunk_size_);

        // 如果剩余数据不足以填充数据包
        if (total_bytes - parsed < bytes)
        {
            // 返回 1 表示需要更多数据
            return 1;
        }

        // 获取当前数据包的写入位置
        const char * body = packet->Data() + packet->PacketSize();
        // 将数据从缓冲区复制到数据包中
        memcpy((void*)body, pos + parsed, bytes);
        // 更新数据包的大小
        packet->UpdatePacketSize(bytes);
        // 更新解析的字节数
        parsed += bytes;
        
        // 从缓冲区中移除已解析的数据
        buff.Retrieve(parsed);
        // 更新剩余字节数
        total_bytes -= parsed;

        // 更新上次的消息头部信息
        prev->cs_id = csid;
        prev->msg_len = msg_len;
        prev->msg_sid = msg_sid;
        prev->msg_type = msg_type;
        prev->timestamp = timestamp;

        // 如果数据包已填满
        if (packet->Space() == 0)
        {
            // 设置数据包的消息类型
            packet->SetPacketType(msg_type);
            // 设置数据包的时间戳
            packet->SetTimestamp(timestamp);
            // 调用 MessageComplete 函数处理完整的消息
            MessageComplete(std::move(packet));
            // 重置数据包指针
            packet.reset();
        }
    }

    // 返回 1 表示解析成功
    return 1;
}

void RtmpContext::MessageComplete(PacketPtr && data)
{
    // 当一个完整的消息包接收完毕时，打印消息类型和消息长度
     RTMP_TRACE << " recv message type : " << data->PacketType() << " len : " << data->PacketSize() << std::endl;
}