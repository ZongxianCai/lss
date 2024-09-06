#include "RtmpContext.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "mmedia/base/BytesWriter.h"

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
        CheckAndSend();
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

bool RtmpContext::BuildChunk(const PacketPtr &packet, uint32_t timestamp, bool fmt0)
{
    // 获取数据包的消息头
    RtmpMsgHeaderPtr h = packet->Ext<RtmpMsgHeader>();

    // 如果消息头存在，开始构建块
    if (h)
    {
        // 将数据包添加到正在发送的队列中
        out_sending_packets_.emplace_back(packet);
        // 获取该 CSID 上次发送的消息头
        RtmpMsgHeaderPtr &prev = out_message_headers_[h->cs_id];
        // 判断是否可以使用时间戳增量，减少数据冗余
        bool use_delta = !fmt0 && !prev && timestamp >= prev->timestamp && h->msg_sid == prev->msg_sid;

        // 如果没有上次的消息头，则创建一个新的
        if (!prev)
        {
            prev = std::make_shared<RtmpMsgHeader>();
        }

        // 默认使用格式0
        int fmt = kRtmpFmt0;
         
        // 如果可以使用增量更新
        if (use_delta)
        {
            // 使用格式1
            fmt = kRtmpFmt1;
            // 计算时间戳的增量
            timestamp -= prev->timestamp;

            // 如果消息类型和长度相同，使用格式2
            if (h->msg_type == prev->msg_type && h->msg_len == prev->msg_len)
            {
                fmt = kRtmpFmt2;

                // 如果增量相同，使用格式3
                if (timestamp == out_deltas_[h->cs_id]) 
                {
                    fmt = kRtmpFmt3;
                }   
            }
        }

        // 设置输出缓冲区当前指针
        char *p = out_current_;

        // 构建基本头部，根据 CSID 来决定如何编码
        if (h->cs_id < 64)
        {
            // 小于64，直接编码
            *p++ = (char)((fmt << 6) | h->cs_id);
        }
        else if (h->cs_id < (64 + 256))
        {
            // 使用1字节编码CSID
           *p++ = (char)((fmt << 6) | 0); 
           *p++ = (char)(h->cs_id - 64);
        }
        else 
        {
            // 使用2字节编码CSID
           *p++ = (char)((fmt << 6) | 1); 
           uint16_t cs = h->cs_id - 64;
           memcpy(p, &cs, sizeof(uint16_t));
           p += sizeof(uint16_t);
        }

        // 时间戳处理，如果超过最大值，则使用最大值
        auto ts = timestamp;

        if (timestamp >= 0xFFFFFF)
        {
            ts = 0xFFFFFF;
        }

        // 根据不同格式写入消息头部信息
        if (fmt == kRtmpFmt0)
        {
             // 格式0：写入时间戳、消息长度、消息类型和消息流ID
            p += BytesWriter::WriteUint24T(p, ts);
            p += BytesWriter::WriteUint24T(p, h->msg_len);
            p += BytesWriter::WriteUint8T(p, h->msg_type);

            memcpy(p, &h->msg_sid, 4);
            p += 4;
            // 重置增量
            out_deltas_[h->cs_id] = 0;
        } 
        else if (fmt == kRtmpFmt1)
        {
            // 格式1：写入时间戳、消息长度和消息类型
            p += BytesWriter::WriteUint24T(p, ts);
            p += BytesWriter::WriteUint24T(p, h->msg_len);
            p += BytesWriter::WriteUint8T(p, h->msg_type);
            out_deltas_[h->cs_id] = timestamp;
        }
        else if (fmt == kRtmpFmt2)
        {
            // 格式2：仅写入时间戳
            p += BytesWriter::WriteUint24T(p, ts);
            // 更新增量
            out_deltas_[h->cs_id] = timestamp;
        }    

        // 如果时间戳达到最大值，写入扩展时间戳
        if (ts == 0xFFFFFF)
        {
            memcpy(p, &timestamp, 4);
            p += 4;
        }    

        // 将构建好的消息头部数据保存到发送缓冲区
        BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_, p-out_current_);
        sending_bufs_.emplace_back(std::move(nheader));
        out_current_ = p;

        // 更新上次的消息头信息
        prev->cs_id = h->cs_id;
        prev->msg_len = h->msg_len;
        prev->msg_sid = h->msg_sid;
        prev->msg_type = h->msg_type;

        // 如果使用格式0，直接更新时间戳；否则，增量更新
        if (fmt == kRtmpFmt0)
        {
            prev->timestamp = timestamp;
        }
        else 
        {
            prev->timestamp += timestamp;
        }
        
        // 处理消息体部分，将数据分块并添加到发送队列中
        // 指向消息体数据的起始位置
        const char *body = packet->Data();
        // 用于跟踪已经处理的数据字节数
        int32_t bytes_parsed = 0;

        // 持续处理消息体数据，直到所有数据被分块处理完毕
        while (true)
        {
            // 计算每次发送的数据块大小
            // 指向当前要处理的消息体数据的开始位置
            const char * chunk = body + bytes_parsed;
            // 表示当前块的大小，等于剩余消息体长度和设定的最大块大小（out_chunk_size_）之间的较小值
            int32_t size = h->msg_len - bytes_parsed;
            size = std::min(size, out_chunk_size_);

            // 创建一个新的 BufferNode 对象，将当前块数据封装进去，并添加到 sending_bufs_ 缓冲区队列中
            BufferNodePtr node = std::make_shared<BufferNode>((void*)chunk, size);
            sending_bufs_.emplace_back(std::move(node));
            // 更新 bytes_parsed，以记录已经处理的数据量
            bytes_parsed += size;

            // 如果数据还未发送完，继续处理
            if (bytes_parsed < h->msg_len)
            {
                // 如果缓冲区不够用，输出错误信息并停止处理
                if (out_current_ - out_buffer_ >= 4096)
                {
                    RTMP_ERROR << " rtmp had no enough out header buffer.";

                    break;
                }

                // 构建后续的数据块头部
                char *p = out_current_;

                // 对于不同的 cs_id 范围，分别使用不同的方式构建头部
                // 如果 Chunk Stream ID (cs_id) 小于 64
                if (h->cs_id < 64)
                {
                    // 将格式3的标志位与 cs_id 结合编码到一个字节中，并存入缓冲区，同时指针 p 向后移动一位
                    *p++ = (char)(0xC0 | h->cs_id);
                }
                // 如果 Chunk Stream ID (cs_id) 在 64 到 319 之间
                else if (h->cs_id < (64 + 256))
                {
                    // 首先将格式3的标志位和高位部分0编码到一个字节中，并存入缓冲区，同时指针 p 向后移动一位
                    *p++ = (char)(0xC0 | 0); 
                    // 将 cs_id 减去 64 的结果编码到第二个字节中，并存入缓冲区，同时指针 p 向后移动一位
                    *p++ = (char)(h->cs_id - 64);
                }
                // 如果 Chunk Stream ID (cs_id) 大于等于 320
                else 
                {
                    // 首先将格式3的标志位和高位部分1编码到一个字节中，并存入缓冲区，同时指针 p 向后移动一位
                    *p++ = (char)(0xC0 | 1); 
                    // 计算出需要编码的 cs_id 值（减去 64）
                    uint16_t cs = h->cs_id - 64;
                    // 将这个 16 位的 cs_id 复制到缓冲区中
                    memcpy(p, &cs, sizeof(uint16_t));
                    // 指针 p 向后移动两个字节，为后续数据存储做准备
                    p += sizeof(uint16_t);
                }

                // 对于时间戳超过最大值的情况，写入扩展时间戳
                if (ts == 0xFFFFFF)
                {
                    memcpy(p, &timestamp, 4);
                    p += 4;
                }      


                // 构建完头部后，将其保存到发送缓冲区，并更新 out_current_ 指针
                BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_, p-out_current_);
                sending_bufs_.emplace_back(std::move(nheader));
                out_current_ = p;      
            }
            else 
            {
                // 数据发送完毕，退出循环
                break;
            }
        }

        // 如果消息体的所有块都成功构建并添加到发送队列，返回 true 表示成功
        return true;
    }

    // 如果消息头不存在，返回失败
    return false;
}

void RtmpContext::Send()
{
    // 如果当前正在发送数据
    if (sending_)
    {
        // 直接返回，不执行后续操作
        return;
    }

    // 标记当前状态为正在发送
    sending_ = true;

    // 最多处理 10 个数据包
    for (int i = 0; i < 10; i++)
    {
        // 如果等待队列为空
        if (out_waiting_queue_.empty())
        {
            // 退出循环，不再处理
            break;
        }

        // 取出等待队列中的第一个数据包
        PacketPtr packet = std::move(out_waiting_queue_.front());
        // 从等待队列中移除该数据包
        out_waiting_queue_.pop_front();
        // 将数据包构建为 RTMP 块
        BuildChunk(std::move(packet));
    }

    // 将准备好的数据块通过连接发送出去
    connection_->Send(sending_bufs_);
}

bool RtmpContext::Ready() const
{
    // 如果当前没有在发送数据，返回 true，表示准备好
    return !sending_;
}

bool RtmpContext::BuildChunk (PacketPtr &&packet, uint32_t timestamp, bool fmt0)
{
    // 获取数据包中的 RTMP 消息头
    RtmpMsgHeaderPtr h = packet->Ext<RtmpMsgHeader>();

    // 如果消息头存在
    if (h)
    {
        // 将数据包添加到正在发送的队列中
        out_sending_packets_.emplace_back(std::move(packet));
        // 获取之前的消息头，用于与当前消息头进行比较
        RtmpMsgHeaderPtr &prev = out_message_headers_[h->cs_id];
        // 检查是否使用时间戳增量（非格式0，之前的消息头存在，且时间戳合法，消息流ID相同）
        bool use_delta = !fmt0 && !prev && timestamp >= prev->timestamp && h->msg_sid == prev->msg_sid;
        
        // 如果之前的消息头不存在，则初始化
        if (!prev)
        {
            prev = std::make_shared<RtmpMsgHeader>();
        }

        // 默认使用格式0
        int fmt = kRtmpFmt0;

        // 如果使用时间戳增量
        if (use_delta)
        {
            // 使用格式1
            fmt = kRtmpFmt1 ;

            // 计算时间戳差值
            timestamp -= prev->timestamp;

            // 如果消息类型和长度相同，使用格式2
            if (h->msg_type == prev->msg_type && h->msg_len == prev->msg_len)
            {
                fmt = kRtmpFmt2;

                // 如果时间戳差值相同，使用格式3
                if (timestamp == out_deltas_[h->cs_id]) 
                {
                    fmt = kRtmpFmt3;
                }   
            }
        }

        // 指向当前缓冲区位置的指针
        char *p = out_current_;

        // 根据消息流 ID 设置 chunk 头部
        // 如果 Chunk Stream ID (cs_id) 小于 64
            if (h->cs_id < 64)
            {
                // 将格式3的标志位与 cs_id 结合编码到一个字节中，并存入缓冲区，同时指针 p 向后移动一位
                *p++ = (char)(0xC0 | h->cs_id);
            }
            // 如果 Chunk Stream ID (cs_id) 在 64 到 319 之间
            else if (h->cs_id < (64 + 256))
            {
                // 首先将格式3的标志位和高位部分0编码到一个字节中，并存入缓冲区，同时指针 p 向后移动一位
                *p++ = (char)(0xC0 | 0); 
                // 将 cs_id 减去 64 的结果编码到第二个字节中，并存入缓冲区，同时指针 p 向后移动一位
                *p++ = (char)(h->cs_id - 64);
            }
            // 如果 Chunk Stream ID (cs_id) 大于等于 320
            else 
            {
                // 首先将格式3的标志位和高位部分1编码到一个字节中，并存入缓冲区，同时指针 p 向后移动一位
                *p++ = (char)(0xC0 | 1); 
                // 计算出需要编码的 cs_id 值（减去 64）
                uint16_t cs = h->cs_id - 64;
                // 将这个 16 位的 cs_id 复制到缓冲区中
                memcpy(p, &cs, sizeof(uint16_t));
                // 指针 p 向后移动两个字节，为后续数据存储做准备
                p += sizeof(uint16_t);
            }

        // 设置时间戳变量
        auto ts = timestamp;

        // 如果时间戳超过最大值，设置为最大值
        if (timestamp >= 0xFFFFFF)
        {
            ts = 0xFFFFFF;
        }

        // 根据格式不同，设置不同的消息头
        if (fmt == kRtmpFmt0)
        {
            // 写入24位时间戳
            p += BytesWriter::WriteUint24T(p, ts);
            // 写入消息长度
            p += BytesWriter::WriteUint24T(p, h->msg_len);
            // 写入消息类型
            p += BytesWriter::WriteUint8T(p, h->msg_type);

            // 写入消息流 ID
            memcpy(p, &h->msg_sid, 4);
            p += 4;
            // 重置增量时间戳
            out_deltas_[h->cs_id] = 0;
        } 
        else if (fmt == kRtmpFmt1)
        {
            // 写入24位时间戳
            p += BytesWriter::WriteUint24T(p, ts);
            // 写入消息长度
            p += BytesWriter::WriteUint24T(p, h->msg_len);
            // 写入消息类型
            p += BytesWriter::WriteUint8T(p, h->msg_type);
            // 更新增量时间戳
            out_deltas_[h->cs_id] = timestamp;
        }
        else if (fmt == kRtmpFmt2)
        {
            // 写入24位时间戳
            p += BytesWriter::WriteUint24T(p, ts);
            // 更新增量时间戳
            out_deltas_[h->cs_id] = timestamp;
        }    

        // 如果时间戳为最大值，写入完整的时间戳
        if (ts == 0xFFFFFF)
        {
            memcpy(p, &timestamp, 4);
            p += 4;
        }    

        // 创建并保存数据块头部
        BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_, p-out_current_);
        sending_bufs_.emplace_back(std::move(nheader));
        out_current_ = p;

        // 更新之前的消息头信息
        prev->cs_id = h->cs_id;
        prev->msg_len = h->msg_len;
        prev->msg_sid = h->msg_sid;
        prev->msg_type = h->msg_type;

        // 更新时间戳
        if (fmt == kRtmpFmt0)
        {
            prev->timestamp = timestamp;
        }
        else 
        {
            prev->timestamp += timestamp;
        }
        
        // 处理消息体，将其分块并添加到发送缓冲区
        const char *body = packet->Data();
        int32_t bytes_parsed = 0;

        while(true)
        {
            // 获取当前数据块
            const char * chunk = body + bytes_parsed;
            // 计算剩余的消息体长度
            int32_t size = h->msg_len - bytes_parsed;
            // 计算本次发送的数据块大小
            size = std::min(size, out_chunk_size_);

            // 创建数据块节点
            BufferNodePtr node = std::make_shared<BufferNode>((void*)chunk, size);
            // 添加到发送缓冲区
            sending_bufs_.emplace_back(std::move(node));
            // 更新已解析的字节数
            bytes_parsed += size;

            // 如果还有数据未发送完
            if (bytes_parsed < h->msg_len)
            {
                // 检查缓冲区是否足够
                if (out_current_ - out_buffer_ >= 4096)
                {
                    // 缓冲区不足时输出错误信息
                    RTMP_ERROR << " rtmp had no enough out header buffer.";

                    break;
                }

                // 获取当前缓冲区指针
                char *p = out_current_;

                // 对于不同的 cs_id 范围，分别使用不同的方式构建头部
                // 如果 Chunk Stream ID (cs_id) 小于 64
                if (h->cs_id < 64)
                {
                    // 将格式3的标志位与 cs_id 结合编码到一个字节中，并存入缓冲区，同时指针 p 向后移动一位
                    *p++ = (char)(0xC0 | h->cs_id);
                }
                // 如果 Chunk Stream ID (cs_id) 在 64 到 319 之间
                else if (h->cs_id < (64 + 256))
                {
                    // 首先将格式3的标志位和高位部分0编码到一个字节中，并存入缓冲区，同时指针 p 向后移动一位
                    *p++ = (char)(0xC0 | 0); 
                    // 将 cs_id 减去 64 的结果编码到第二个字节中，并存入缓冲区，同时指针 p 向后移动一位
                    *p++ = (char)(h->cs_id - 64);
                }
                // 如果 Chunk Stream ID (cs_id) 大于等于 320
                else 
                {
                    // 首先将格式3的标志位和高位部分1编码到一个字节中，并存入缓冲区，同时指针 p 向后移动一位
                    *p++ = (char)(0xC0 | 1); 
                    // 计算出需要编码的 cs_id 值（减去 64）
                    uint16_t cs = h->cs_id - 64;
                    // 将这个 16 位的 cs_id 复制到缓冲区中
                    memcpy(p, &cs, sizeof(uint16_t));
                    // 指针 p 向后移动两个字节，为后续数据存储做准备
                    p += sizeof(uint16_t);
                }

                // 如果时间戳为最大值，写入完整的时间戳
                if (ts == 0xFFFFFF)
                {
                    memcpy(p, &timestamp, 4);
                    p += 4;
                }      

                BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_, p-out_current_);
                sending_bufs_.emplace_back(std::move(nheader));
                out_current_ = p;      
            }
            else 
            {
                break;
            }
        }

        return true;
    }

    return false;
}

void RtmpContext::CheckAndSend()
{
    sending_ = false;
    out_current_ = out_buffer_;
    sending_bufs_.clear();
    out_sending_packets_.clear();

    if (!out_waiting_queue_.empty())
    {
        Send();
    }
    else
    {
        if (rtmp_handler_)
        {
            rtmp_handler_->OnActive(connection_);
        }
    }
}

void RtmpContext::PushOutQueue(PacketPtr &&packet)
{
    out_waiting_queue_.emplace_back(std::move(packet));
    Send();
}