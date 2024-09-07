#include "RtmpContext.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "mmedia/base/BytesWriter.h"
#include "mmedia/rtmp/amf/AMFObject.h"
#include "base/StringUtils.h"

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
        auto r = ParseMessage(buff);

        // 获取缓冲区可读字节数
        last_left_ = buff.ReadableBytes();

        // 如果当前状态是消息处理阶段，调用消息解析函数
        // return ParseMessage(buff);

        // 如果缓冲区还有可读数据，继续解析
        return r;
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

    // 获取当前缓冲区指针
    in_bytes_ += (buff.ReadableBytes() - last_left_);

    // 计算缓冲区内可读字节数
    SendBytesRecv();

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
    
    // 获取数据包的消息类型
    // 获取数据包的类型
    auto type = data->PacketType();

    switch (type)
    {
        // 处理 RTMP 消息类型为 Chunk Size（分块大小）的数据包
        case kRtmpMsgTypeChunkSize:
        {
            // 调用 HandleChunkSize 函数处理分块大小消息
            HandleChunkSize(data);
            break;
        }
        
        // 处理 RTMP 消息类型为 Bytes Read（字节读取）的数据包
        case kRtmpMsgTypeBytesRead:
        {
            // 输出日志信息，表示接收到字节读取消息
            RTMP_TRACE << " message bytes read recv.";
            break;
        }        
        
        // 处理 RTMP 消息类型为 User Control（用户控制）的数据包
        case kRtmpMsgTypeUserControl:
        {
            // 调用 HandleUserMessage 函数处理用户控制消息
            HandleUserMessage(data);
            break;
        }
        
        // 处理 RTMP 消息类型为 Window ACK Size（窗口确认大小）的数据包
        case kRtmpMsgTypeWindowACKSize:
        {
            // 调用 HandleAckWindowSize 函数处理窗口确认大小消息
            HandleAckWindowSize(data);
            break;
        }

        // 处理 AMF3 消息类型
        case kRtmpMsgTypeAMF3Message:
        {
            // 调用 HandleAmfCommand 函数来处理 AMF3 类型的命令消息
            HandleAmfCommand(data, true);
            break;
        }

        // 处理 AMF 消息类型
        case kRtmpMsgTypeAMFMessage:
        {
            // 调用 HandleAmfCommand 函数来处理 AMF 类型的命令消息
            HandleAmfCommand(data);
            break;
        }
        
        // 处理不支持的消息类型
        default:
            // 输出错误日志，表示收到的消息类型不被支持
            RTMP_ERROR << " not surpport message type : " << type;
            break;
    }

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

                // 创建并保存后续数据块头部
                BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_, p-out_current_);
                sending_bufs_.emplace_back(std::move(nheader));
                // 更新缓冲区指针
                out_current_ = p;      
            }
            else 
            {
                // 数据发送完毕，退出循环
                break;
            }
        }

        // 构建成功，返回 true
        return true;
    }

    // 消息头不存在，返回 false
    return false;
}

void RtmpContext::CheckAndSend()
{
    // 将发送标志设置为 false，表示当前不再发送数据
    sending_ = false;
    // 重置当前缓冲区指针到缓冲区的起始位置
    out_current_ = out_buffer_;
    // 清空正在发送的缓冲区
    sending_bufs_.clear();
    // 清空正在发送的数据包列表
    out_sending_packets_.clear();

    // 如果等待发送队列不为空
    if (!out_waiting_queue_.empty())
    {
        // 调用 Send() 方法开始发送数据
        Send();
    }
    else    // 如果等待发送队列为空
    {
        // 检查是否存在 RTMP 处理器
        if (rtmp_handler_)
        {
            // 调用处理器的 OnActive 方法，表示当前连接活跃
            rtmp_handler_->OnActive(connection_);
        }
    }
}

void RtmpContext::PushOutQueue(PacketPtr &&packet)
{
    // 将传入的数据包添加到等待发送队列中
    out_waiting_queue_.emplace_back(std::move(packet));

    // 立即调用 Send() 方法开始处理发送
    Send();
}

void RtmpContext::SendSetChunkSize()
{
    // 创建一个新的数据包，初始大小为 64 字节
    PacketPtr packet = Packet::NewPacket(64);

    // 创建一个 RTMP 消息头的共享指针
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    // 如果消息头创建成功
    if (header)
    {
        header->cs_id = kRtmpCSIDCommand;           // 设置 Chunk Stream ID 为 RTMP 命令类型的固定 ID
        header->msg_len = 0;                        // 初始化消息长度为 0，稍后会设置
        header->msg_type = kRtmpMsgTypeChunkSize;   // 设置消息类型为设置 Chunk Size 类型
        header->timestamp = 0;                      // 设置时间戳为 0
        header->msg_sid = kRtmpMsID0;               // 设置消息 Stream ID 为 0，表示系统控制消息
        packet->SetExt(header);                     // 将消息头附加到数据包中
    }

    // 获取数据包的指针
    char *body = packet->Data();

    // 将当前的 Chunk Size 写入到数据包的 body 部分
    header->msg_len = BytesWriter::WriteUint32T(body, out_chunk_size_);

    // 设置数据包的实际大小
    packet->SetPacketSize(header->msg_len);

    // 打印调试信息，输出当前发送的 Chunk Size 以及目标主机的 IP 和端口
    RTMP_DEBUG << " send chuck size : " << out_chunk_size_ << " to host : " << connection_->PeerAddr().ToIpPort();

    // 将数据包放入发送队列，并触发发送
    PushOutQueue(std::move(packet));
}

void RtmpContext::SendAckWindowSize()
{
    // 创建一个新的数据包，初始大小为 64 字节
    PacketPtr packet = Packet::NewPacket(64);

    // 创建一个 RTMP 消息头的共享指针
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    // 如果消息头创建成功
    if (header)
    {
        header->cs_id = kRtmpCSIDCommand;               // 设置 Chunk Stream ID 为 RTMP 命令类型的固定 ID
        header->msg_len = 0;                            // 初始化消息长度为 0，稍后会设置
        header->msg_type = kRtmpMsgTypeWindowACKSize;   // 设置消息类型为窗口确认大小
        header->timestamp = 0;                          // 设置时间戳为 0
        header->msg_sid = kRtmpMsID0;                   // 设置消息 Stream ID 为 0，表示系统控制消息
        packet->SetExt(header);                         // 将消息头附加到数据包中
    }

    // 获取数据包的指针，用于写入数据
    char *body = packet->Data();
    
    // 将当前的确认窗口大小写入到数据包的 body 部分
    header->msg_len = BytesWriter::WriteUint32T(body, ack_size_);
    
    // 设置数据包的实际大小
    packet->SetPacketSize(header->msg_len);

    // 打印调试信息，输出当前发送的确认窗口大小以及目标主机的 IP 和端口
    RTMP_DEBUG << " send act size : " << ack_size_ << " to host : " << connection_->PeerAddr().ToIpPort();
    
    // 将数据包放入发送队列，并触发发送
    PushOutQueue(std::move(packet));
}

void RtmpContext::SendSetPeerBandwidth()
{
    // 创建一个新的数据包，初始大小为 64 字节
    PacketPtr packet = Packet::NewPacket(64);

    // 创建一个 RTMP 消息头的共享指针
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    if (header)  // 如果消息头创建成功
    {
        header->cs_id = kRtmpCSIDCommand;  // 设置 Chunk Stream ID 为 RTMP 命令类型的固定 ID
        header->msg_len = 0;  // 初始化消息长度为 0，稍后会设置
        header->msg_type = kRtmpMsgTypeSetPeerBW;  // 设置消息类型为 Set Peer Bandwidth
        header->timestamp = 0;  // 设置时间戳为 0
        header->msg_sid = kRtmpMsID0;  // 设置消息 Stream ID 为 0，表示系统控制消息
        packet->SetExt(header);  // 将消息头附加到数据包中
    }

    // 获取数据包的指针，用于写入数据
    char *body = packet->Data();
    
    // 将当前的确认窗口大小写入到数据包的 body 部分
    body += BytesWriter::WriteUint32T(body, ack_size_);
    
    // 设置限制类型，0x02 表示动态（type = 2）
    *body++ = 0x02;

    // 设置数据包的实际大小为 5 字节（4 字节的确认窗口大小 + 1 字节的限制类型）
    packet->SetPacketSize(5);

    // 打印调试信息，输出当前发送的带宽限制以及目标主机的 IP 和端口
    RTMP_DEBUG << " send bandwidth : " << ack_size_ << " to host : " << connection_->PeerAddr().ToIpPort();
    
    // 将数据包放入发送队列，并触发发送
    PushOutQueue(std::move(packet));
}

void RtmpContext::SendBytesRecv()
{
    // 如果接收的字节数大于等于确认窗口大小
    if (in_bytes_ >= ack_size_)
    {
        // 创建一个新的数据包，初始大小为 64 字节
        PacketPtr packet = Packet::NewPacket(64);

        // 创建一个 RTMP 消息头的共享指针
        RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

        // 如果消息头创建成功
        if (header)
        {
            header->cs_id = kRtmpCSIDCommand;  // 设置 Chunk Stream ID 为 RTMP 命令类型的固定 ID
            header->msg_len = 0;  // 初始化消息长度为 0，稍后会设置
            header->msg_type = kRtmpMsgTypeBytesRead;  // 设置消息类型为 Bytes Received (BytesRead) 消息
            header->timestamp = 0;  // 设置时间戳为 0
            header->msg_sid = kRtmpMsID0;  // 设置消息 Stream ID 为 0，表示系统控制消息
            packet->SetExt(header);  // 将消息头附加到数据包中
        }

        // 获取数据包的指针，用于写入数据
        char *body = packet->Data();
        
        // 将已接收的字节数写入到数据包的 body 部分，并更新消息长度
        header->msg_len = BytesWriter::WriteUint32T(body, in_bytes_);

        // 设置数据包的实际大小为消息长度
        packet->SetPacketSize(header->msg_len);

        // RTMP_DEBUG << " send act size : " << ack_size_ << " to host : " << connection_->PeerAddr().ToIpPort();

        // 将数据包放入发送队列，并触发发送
        PushOutQueue(std::move(packet));

        // 重置接收字节计数
        in_bytes_ = 0;
    }
}

void RtmpContext::SendUserCtrlMessage(short nType, uint32_t value1, uint32_t value2)
{
    // 创建一个新的数据包，初始大小为 64 字节
    PacketPtr packet = Packet::NewPacket(64);
    
    // 创建一个 RTMP 消息头的共享指针
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    
    // 如果消息头创建成功
    if (header)
    {
        header->cs_id = kRtmpCSIDCommand;  // 设置 Chunk Stream ID 为 RTMP 命令类型的固定 ID
        header->msg_len = 0;  // 初始化消息长度为 0，稍后会根据写入的数据内容进行设置
        header->msg_type = kRtmpMsgTypeUserControl;  // 设置消息类型为 User Control Message
        header->timestamp = 0;  // 设置时间戳为 0
        header->msg_sid = kRtmpMsID0;  // 设置消息 Stream ID 为 0，表示系统控制消息
        packet->SetExt(header);  // 将消息头附加到数据包中
    }

    // 获取数据包的指针，用于写入数据
    char *body = packet->Data();
    char *p = body;

    // 写入用户控制消息类型 (nType)
    p += BytesWriter::WriteUint16T(body, nType);
    
    // 写入第一个值 (value1)，通常用于标识用户控制消息的相关信息
    p += BytesWriter::WriteUint32T(body, value1);

    // 如果消息类型是设置缓冲区长度，则还需要写入第二个值 (value2)
    if (nType == kRtmpEventTypeSetBufferLength)
    {
        p += BytesWriter::WriteUint32T(body, value2);
    }

    // 设置数据包的实际大小为已写入的字节数
    packet->SetPacketSize(header->msg_len);

    // 打印调试信息，显示发送的用户控制消息的类型和值
    RTMP_DEBUG << " send user control type : " << nType << " value : " << value1 << " value2 : " << value2 << " to host : " << connection_->PeerAddr().ToIpPort();

    // 将数据包放入发送队列，并触发发送
    PushOutQueue(std::move(packet));
}

void RtmpContext::HandleChunkSize(PacketPtr &packet)
{
    // 检查数据包的大小是否至少为 4 字节，这是一个有效 Chunk Size 消息所需的最小长度
    if (packet->PacketSize() >= 4)
    {
        // 从数据包的内容中读取 4 字节无符号整数，即 Chunk Size 的新值
        auto size = BytesReader::ReadUint32T(packet->Data());
        
        // 输出调试信息，显示当前的 Chunk Size 以及即将更新的新值
        RTMP_DEBUG << " recv chunk size in_chunk_size : " << in_chunk_size_ << " change to : " << size;
        
        // 更新当前的 Chunk Size 值
        in_chunk_size_ = size;
    }
    else 
    {
        // 如果数据包的大小不足 4 字节，输出错误信息，提示数据包无效
        RTMP_ERROR << " invalid chunk size packet msg_len : " << packet->PacketSize() << " host : " << connection_->PeerAddr().ToIpPort();
    }
}

void RtmpContext::HandleAckWindowSize(PacketPtr &packet)
{
    // 检查数据包的大小是否至少为 4 字节，这是一个有效的 Ack Window Size 消息所需的最小长度
    if (packet->PacketSize() >= 4)
    {
        // 从数据包的内容中读取 4 字节无符号整数，即 Ack Window Size 的新值
        auto size = BytesReader::ReadUint32T(packet->Data());
        
        // 输出调试信息，显示当前的 Ack Window Size 以及即将更新的新值
        RTMP_DEBUG << " recv ack window size ack_size_ : " << ack_size_ << " change to : " << size;
        
        // 更新当前的 Ack Window Size 值
        ack_size_ = size;
    }
    else 
    {
        // 如果数据包的大小不足 4 字节，输出错误信息，提示数据包无效
        RTMP_ERROR << " invalid ack window size packet msg_len : " << packet->PacketSize() << " host : " << connection_->PeerAddr().ToIpPort();
    }    
}

void RtmpContext::HandleUserMessage(PacketPtr &packet)
{
    // 获取数据包的大小
    auto msg_len = packet->PacketSize();

    // 如果数据包的大小小于 6 字节，认为该用户控制消息无效，输出错误日志并返回
    if (msg_len < 6)
    {
        RTMP_ERROR << " invalid user control packet msg_len : " << packet->PacketSize() << " host : " << connection_->PeerAddr().ToIpPort();
        return;
    }

    // 获取数据包的内容指针
    char * body = packet->Data();

    // 读取前 2 字节，作为消息类型
    auto type = BytesReader::ReadUint16T(body);

    // 读取接下来的 4 字节，作为消息值
    auto value = BytesReader::ReadUint32T(body + 2);

    // 输出接收到的用户控制消息的类型和值
    RTMP_TRACE << " recv user control type : " << type << " value : " << value << " host : " << connection_->PeerAddr().ToIpPort();

    // 根据消息类型进行处理
    switch (type)
    {
        // 处理 Stream Begin 消息
        case kRtmpEventTypeStreamBegin:
        {
            RTMP_TRACE << " recv stream begin value : " << value << " host : " << connection_->PeerAddr().ToIpPort();
            break;
        }
        // 处理 Stream EOF 消息
        case kRtmpEventTypeStreamEOF:
        {
            RTMP_TRACE << " recv stream eof value : " << value << " host : " << connection_->PeerAddr().ToIpPort();
            break;
        }   
        // 处理 Stream Dry 消息
        case kRtmpEventTypeStreamDry:
        {
            RTMP_TRACE << " recv stream dry value : " << value << " host : " << connection_->PeerAddr().ToIpPort();
            break;
        }
        // 处理 Set Buffer Length 消息
        case kRtmpEventTypeSetBufferLength:
        {
            RTMP_TRACE << " recv set buffer length value : " << value << " host : " << connection_->PeerAddr().ToIpPort();

            // 进一步检查数据包长度是否足够，处理 Set Buffer Length 需要至少 10 字节的数据包
            if (msg_len < 10)
            {
                RTMP_ERROR << " invalid user control packet msg_len : " << packet->PacketSize() << " host : " << connection_->PeerAddr().ToIpPort();                
                return ;
            }
            break;
        }   
        // 处理 Streams Recorded 消息
        case kRtmpEventTypeStreamsRecorded:
        {
            RTMP_TRACE << " recv stream recorded value : " << value << " host : " << connection_->PeerAddr().ToIpPort();
            break;
        }
        // 处理 Ping Request 消息
        case kRtmpEventTypePingRequest:
        {
            RTMP_TRACE << " recv ping request value : " << value << " host : " << connection_->PeerAddr().ToIpPort();
            // 发送 Ping Response 消息
            SendUserCtrlMessage(kRtmpEventTypePingResponse, value, 0);
            break;
        }   
        // 处理 Ping Response 消息
        case kRtmpEventTypePingResponse:
        {
            RTMP_TRACE << " recv ping response value : " << value << " host : " << connection_->PeerAddr().ToIpPort();
            break;
        }
        // 默认处理，忽略未识别的用户控制消息类型
        default:
            break;                             
    }
}

void RtmpContext::HandleAmfCommand(PacketPtr &data, bool amf3)
{
    // 打印 AMF 消息的长度和来源主机地址
    RTMP_TRACE << " amf message len : " << data->PacketSize() << " host : " << connection_->PeerAddr().ToIpPort();

    // 获取消息体的起始位置
    const char *body = data->Data();

    // 获取消息的总长度
    int32_t msg_len = data->PacketSize();

    // 如果是 AMF3 消息，跳过第一个字节（AMF3 消息格式的标志位）
    if (amf3)
    {
        // 跳过一个字节
        body += 1;

        // 减去一个字节的长度
        msg_len -= 1;
    }

    // 创建 AMFObject 对象用于解码 AMF 消息
    AMFObject obj;

    // 尝试解码消息体
    if (obj.Decode(body, msg_len) < 0)
    {
        // 解码失败，打印错误信息
        RTMP_ERROR << " amf decode failed. host : " << connection_->PeerAddr().ToIpPort();

        // 退出函数
        return;
    }

    // 打印解码后的 AMF 对象内容
    obj.Dump();
}

// ------------------------------ 命令解析与实现部分 ------------------------------
void RtmpContext::SendConnect()
{
    // 发送设置Chunk大小的消息
    SendSetChunkSize();

    // 创建一个新的Packet，大小为1024字节
    PacketPtr packet = Packet::NewPacket(1024);

    // 创建一个新的RTMP消息头对象，并初始化头部信息
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    // 设置Chunk Stream ID为AMF初始化流的ID
    header->cs_id = kRtmpCSIDAMFIni;

    // 消息流ID设为0
    header->msg_sid = 0;

    // 消息长度初始为0
    header->msg_len = 0;

    // 设置消息类型为AMF消息
    header->msg_type = kRtmpMsgTypeAMFMessage;

    // 将消息头部设置到Packet中
    packet->SetExt(header);

    // 获取Packet的Data指针，指向消息体的开始位置
    char *body = packet->Data();
    char *p = body;

    // 编码AMF字符串"connect"，表示RTMP的connect命令
    p += AMFAny::EncodeString(p, "connect");

    // 编码AMF数字1.0，表示命令的事务ID
    p += AMFAny::EncodeNumber(p, 1.0);

    // 添加AMF对象的开始标志符
    *p++ = kAMFObject;

    // 编码并添加"app"字段，值为app_，表示应用程序名称
    p += AMFAny::EncodeNamedString(p, "app", app_);

    // 编码并添加"tcUrl"字段，值为tc_url_，表示tcURL
    p += AMFAny::EncodeNamedString(p, "tcUrl", tc_url_);

    // 编码并添加"fpad"字段，值为false，表示是否使用Flash Player后台下载
    p += AMFAny::EncodeNamedBoolean(p, "fpad", false);

    // 编码并添加"capabilities"字段，值为31.0，表示客户端能力
    p += AMFAny::EncodeNamedNumber(p, "capabilities", 31.0);

    // 编码并添加"audioCodecs"字段，值为1639.0，表示音频编解码器支持的格式
    p += AMFAny::EncodeNamedNumber(p, "audioCodecs", 1639.0);

    // 编码并添加"videoCodecs"字段，值为252.0，表示视频编解码器支持的格式
    p += AMFAny::EncodeNamedNumber(p, "videoCodecs", 252.0);

    // 编码并添加"videoFunction"字段，值为1.0，表示视频功能
    p += AMFAny::EncodeNamedNumber(p, "videoFunction", 1.0);

    // 结束AMF对象的编码，添加对象结束符号0x00 0x00 0x09
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;

    // 计算并设置消息长度
    header->msg_len = p - body;

    // 设置Packet的大小
    packet->SetPacketSize(header->msg_len);

    // 输出日志，记录发送的connect消息长度和目标主机信息
    RTMP_TRACE << " send connect msg_len : " << header->msg_len << " to host : " << connection_->PeerAddr().ToIpPort();

    // 将Packet推送到发送队列中
    PushOutQueue(std::move(packet));
}

void RtmpContext::HandleConnect(AMFObject &obj)
{
    // 定义一个变量，表示是否使用AMF3编码，初始值为false
    auto amf3 = false;

    // 从AMF对象中获取"tcUrl"属性，并将其保存到成员变量tc_url_中
    tc_url_ = obj.Property("tcUrl")->String();

    // 获取AMF对象中的第2个属性，该属性本身也是一个AMF对象
    AMFObjectPtr sub_obj = obj.Property(2)->Object();

    // 如果子对象存在
    if (sub_obj)
    {
        // 获取子对象中的"app"属性，并将其保存到成员变量app_中
        app_ = sub_obj->Property("app")->String();

        // 检查子对象中是否有"objectEncoding"属性
        if (sub_obj->Property("objectEncoding"))
        {
            // 如果"objectEncoding"属性值为3.0，表示使用AMF3编码
            amf3 = sub_obj->Property("objectEncoding")->Number() == 3.0;
        }
    }

    // 输出日志，记录接收到的连接信息，包括tcUrl、app名称和是否使用AMF3编码
    RTMP_TRACE << " recv connect tcUrl : " << tc_url_ << " app : " << app_ << " amf3 : " << amf3;

    // 创建一个新的Packet，大小为1024字节
    PacketPtr packet = Packet::NewPacket(1024);

    // 创建一个新的RTMP消息头对象，并初始化头部信息
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    // 设置Chunk Stream ID为AMF初始化流的ID
    header->cs_id = kRtmpCSIDAMFIni;  

    // 消息流ID设为0      
    header->msg_sid = 0;

    // 消息长度初始为0              
    header->msg_len = 0;  

    // 设置消息类型为AMF消息                  
    header->msg_type = kRtmpMsgTypeAMFMessage; 

    // 将消息头部设置到Packet中
    packet->SetExt(header);                 

    // 获取Packet的Data指针，指向消息体的开始位置
    char *body = packet->Data();
    char *p = body;

    // 编码AMF字符串"_result"，表示这是一个响应结果
    p += AMFAny::EncodeString(p, "_result");

    // 编码AMF数字1.0，表示事务ID
    p += AMFAny::EncodeNumber(p, 1.0);

    // 添加AMF对象的开始标志符
    *p++ = kAMFObject;

    // 编码并添加"fmsVer"字段，值为"FMS/3,0,1,123"，表示FMS的版本号
    p += AMFAny::EncodeNamedString(p, "fmsVer", "FMS/3,0,1,123");

    // 编码并添加"capabilities"字段，值为31，表示服务器能力
    p += AMFAny::EncodeNamedNumber(p, "capabilities", 31);

    // 结束AMF对象的编码，添加对象结束符号0x00 0x00 0x09
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;

    // 再次开始一个新的AMF对象
    *p++ = kAMFObject;

    // 编码并添加"level"字段，值为"status"，表示状态信息
    p += AMFAny::EncodeNamedString(p, "level", "status");

    // 编码并添加"code"字段，值为"NetConnection.Connect.Success"，表示连接成功的代码
    p += AMFAny::EncodeNamedString(p, "code", "NetConnection.Connect.Success");

    // 编码并添加"description"字段，值为"Connection succeeded."，表示连接成功的描述信息
    p += AMFAny::EncodeNamedString(p, "description", "Connection succeeded.");

    // 编码并添加"objectEncoding"字段，值为3.0或0，表示是否使用AMF3编码
    p += AMFAny::EncodeNamedNumber(p, "objectEncoding", amf3 ? 3.0 : 0);

    // 结束AMF对象的编码，添加对象结束符号0x00 0x00 0x09
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;

    // 计算并设置消息长度
    header->msg_len = p - body;

    // 设置Packet的大小
    packet->SetPacketSize(header->msg_len);

    // 输出日志，记录发送的连接结果消息长度和目标主机信息
    RTMP_TRACE << " connect result msg_len : " << header->msg_len << " to host : " << connection_->PeerAddr().ToIpPort();
    
    // 将Packet推送到发送队列中
    PushOutQueue(std::move(packet));
}

void RtmpContext::SendCreateStream()
{
    // 创建一个新的Packet，大小为1024字节
    PacketPtr packet = Packet::NewPacket(1024);

    // 创建一个新的RTMP消息头对象，并初始化头部信息
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    // 设置Chunk Stream ID为AMF初始化流的ID
    header->cs_id = kRtmpCSIDAMFIni;

    // 消息流ID设为0
    header->msg_sid = 0;

    // 消息长度初始为0
    header->msg_len = 0;

    // 设置消息类型为AMF消息
    header->msg_type = kRtmpMsgTypeAMFMessage; 

    // 将消息头部设置到Packet中
    packet->SetExt(header);

    // 获取Packet的Data指针，指向消息体的开始位置
    char *body = packet->Data();
    char *p = body;

    // 编码AMF字符串"createStream"，表示创建流的命令
    p += AMFAny::EncodeString(p, "createStream");

    // 编码AMF数字4.0，表示事务ID
    p += AMFAny::EncodeNumber(p, 4.0);

    // 添加AMF空值，表示没有附加参数
    *p++ = kAMFNull;

    // 计算并设置消息长度
    header->msg_len = p - body;

    // 设置Packet的大小
    packet->SetPacketSize(header->msg_len);

    // 输出日志，记录发送的创建流消息的长度和目标主机信息
    RTMP_TRACE << " send create stream msg_len : " << header->msg_len << " to host : " << connection_->PeerAddr().ToIpPort();
    
    // 将Packet推送到发送队列中
    PushOutQueue(std::move(packet));
}

void RtmpContext::HandleCreateStream(AMFObject &obj)
{
    // 从AMF对象中获取事务ID（transaction ID）
    auto tran_id = obj.Property(1)->Number();

    // 创建一个新的Packet，大小为1024字节
    PacketPtr packet = Packet::NewPacket(1024);

    // 创建一个新的RTMP消息头对象，并初始化头部信息
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    // 设置Chunk Stream ID为AMF初始化流的ID
    header->cs_id = kRtmpCSIDAMFIni;

    // 消息流ID设为0
    header->msg_sid = 0;

    // 消息长度初始为0
    header->msg_len = 0;

    // 设置消息类型为AMF消息
    header->msg_type = kRtmpMsgTypeAMFMessage;

    // 将消息头部设置到Packet中
    packet->SetExt(header);

    // 获取Packet的Data指针，指向消息体的开始位置
    char *body = packet->Data();
    char *p = body;

    // 编码AMF字符串"_result"，表示结果消息
    p += AMFAny::EncodeString(p, "_result");

    // 编码事务ID，将其发送回客户端
    p += AMFAny::EncodeNumber(p, tran_id);

    // 添加AMF空值，表示没有附加参数
    *p++ = kAMFNull;

    // 编码消息流ID（MSID），此处为1，表示成功创建的流的ID
    p += AMFAny::EncodeNumber(p, kRtmpMsID1);

    // 计算并设置消息长度
    header->msg_len = p - body;

    // 设置Packet的大小
    packet->SetPacketSize(header->msg_len);

    // 输出日志，记录发送的创建流结果消息的长度和目标主机信息
    RTMP_TRACE << " create stream result msg_len : " << header->msg_len << " to host : " << connection_->PeerAddr().ToIpPort();
    
    // 将Packet推送到发送队列中
    PushOutQueue(std::move(packet));
}

void RtmpContext::SendStatus(const std::string &level, const std::string &code, const std::string &description)
{
    // 创建一个新的Packet对象，大小为1024字节
    PacketPtr packet = Packet::NewPacket(1024);

    // 创建一个新的RTMP消息头对象，并初始化头部信息
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    // 设置Chunk Stream ID为AMF初始化流的ID
    header->cs_id = kRtmpCSIDAMFIni;

    // 消息流ID设为1
    header->msg_sid = 1;

    // 消息长度初始为0
    header->msg_len = 0;

    // 设置消息类型为AMF消息
    header->msg_type = kRtmpMsgTypeAMFMessage;

    // 将消息头部设置到Packet中
    packet->SetExt(header);

    // 获取Packet的Data指针，指向消息体的开始位置
    char *body = packet->Data();
    char *p = body;

    // 编码AMF字符串"onStatus"，表示状态消息
    p += AMFAny::EncodeString(p, "onStatus");

    // 编码数字0，作为事务ID
    p += AMFAny::EncodeNumber(p, 0);

    // 添加AMF空值，表示没有附加参数
    *p++ = kAMFNull;

    // 添加AMF对象开始标识
    *p++ = kAMFObject;

    // 编码并添加"level"字段及其值
    p += AMFAny::EncodeNamedString(p, "level", level);

    // 编码并添加"code"字段及其值
    p += AMFAny::EncodeNamedString(p, "code", code);

    // 编码并添加"description"字段及其值
    p += AMFAny::EncodeNamedString(p, "description", description);

    // 添加AMF对象结束标识
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;

    // 计算并设置消息长度
    header->msg_len = p - body;

    // 设置Packet的大小
    packet->SetPacketSize(header->msg_len);

    // 输出日志，记录发送的状态消息的等级、代码、描述和目标主机信息
    RTMP_TRACE << " send status level : " << level << " code : " << code << " desc : " << description << " to host : " << connection_->PeerAddr().ToIpPort();
    
    // 将Packet推送到发送队列中
    PushOutQueue(std::move(packet));
}

void RtmpContext::SendPlay()
{
    // 创建一个新的Packet对象，大小为1024字节
    PacketPtr packet = Packet::NewPacket(1024);

    // 创建一个新的RTMP消息头对象，并初始化头部信息
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    // 设置Chunk Stream ID为AMF初始化流的ID
    header->cs_id = kRtmpCSIDAMFIni;

    // 消息流ID设为1
    header->msg_sid = 1;

    // 消息长度初始为0
    header->msg_len = 0;

    // 设置消息类型为AMF消息
    header->msg_type = kRtmpMsgTypeAMFMessage;

    // 将消息头部设置到Packet中
    packet->SetExt(header);

    // 获取Packet的Data指针，指向消息体的开始位置
    char *body = packet->Data();
    char *p = body;

    // 编码AMF字符串"play"，表示播放命令
    p += AMFAny::EncodeString(p, "play");

    // 编码数字0，作为事务ID
    p += AMFAny::EncodeNumber(p, 0);

    // 添加AMF空值，表示没有附加参数
    *p++ = kAMFNull;

    // 编码并添加播放的流名称
    p += AMFAny::EncodeString(p, name_);

    // 编码并添加播放的开始位置，-1000表示从头开始播放
    p += AMFAny::EncodeNumber(p, -1000.0);

    // 计算并设置消息长度
    header->msg_len = p - body;

    // 设置Packet的大小
    packet->SetPacketSize(header->msg_len);

    // 输出日志，记录发送的播放命令及目标主机信息
    RTMP_TRACE << " send play name : "<< name_ << " msg_len : " << header->msg_len << " to host : " << connection_->PeerAddr().ToIpPort();
    
    // 将Packet推送到发送队列中
    PushOutQueue(std::move(packet));
}

void RtmpContext::HandlePlay(AMFObject &obj)
{
    // 获取AMF对象中第二个属性的值，并将其作为事务ID
    auto tran_id = obj.Property(1)->Number();

    // 从AMF对象的第四个属性中获取播放的流名称，并赋值给name_
    name_ = obj.Property(3)->String();

    // 解析流名称和tcUrl
    ParseNameAndTcUrl();

    // 输出日志，记录接收到的播放命令的session_name、param和目标主机信息
    RTMP_TRACE << " recv play session_name : " << session_name_ << " param : " << param_ << " host : " << connection_->PeerAddr().ToIpPort();

    // 设置is_player_为true，表示当前连接为播放端
    is_player_ = true;

    // 发送Stream Begin用户控制消息，通知客户端流已经开始
    SendUserCtrlMessage(kRtmpEventTypeStreamBegin, 1, 0);
    
    // 发送状态消息，通知客户端播放已开始
    SendStatus("status", "NetStream.Play.Start", "Start Playing");

    // 如果存在RTMP处理器，调用其OnPlay回调函数，传递连接、会话名称和参数信息
    if (rtmp_handler_)
    {
        rtmp_handler_->OnPlay(connection_, session_name_, param_);
    }
}

void RtmpContext::ParseNameAndTcUrl()
{
    // 找到app_中第一个出现的'/'字符位置
    auto pos = app_.find_first_of("/");

    // 如果找到'/'字符，将app_截取为从该字符后的部分
    if (pos != std::string::npos)
    {
        app_ = app_.substr(pos + 1);
    }

    // 清空param_，用于存储后续从name_中解析的参数部分
    param_.clear();

    // 找到name_中第一个出现的'?'字符位置
    pos = name_.find_first_of("?");

    // 如果找到'?'字符，将name_按该位置分割，前半部分作为流名称，后半部分作为参数
    if (pos != std::string::npos)
    {
        param_ = name_.substr(pos + 1);  // 提取参数部分
        name_ = name_.substr(0, pos);    // 提取流名称部分
    }

    std::string domain;  // 用于存储解析出的域名部分

    // 使用自定义的字符串工具函数，将tc_url_按'/'分割成多个部分存储到list中
    std::vector<std::string> list = base::StringUtils::SplitString(tc_url_, "/");

    // 如果分割出的list有6个元素，说明格式可能为rmtp://ip/domain:port/app/stream
    if (list.size() == 6)
    {
        domain = list[3];  // 提取域名部分
        app_ = list[4];    // 提取app部分
        name_ = list[5];   // 提取流名称部分
    }

    // 如果domain仍为空且tc_url_的长度大于7（去掉"rtmp://"后的长度）
    if (domain.empty() && tc_url_.size() > 7)
    {
        // 查找从第7个字符开始的'/'或':'字符位置
        auto pos = tc_url_.find_first_of(":/", 7);

        // 如果找到，将域名部分提取出来
        if (pos != std::string::npos)
        {
            domain = tc_url_.substr(7, pos);  // 提取域名部分
        }
    }

    session_name_.clear();  // 清空session_name_，准备重新拼接

    // 按照"domain/app/name"的格式拼接会话名称
    session_name_ += domain;
    session_name_ += "/";
    session_name_ += app_;
    session_name_ += "/";
    session_name_ += name_;

    // 输出日志，记录session_name和param，以及目标主机信息
    RTMP_TRACE << " session_name : " << session_name_ << " param : " << param_ << " host : " << connection_->PeerAddr().ToIpPort();
}

void RtmpContext::SendPublish()
{
    // 创建一个新的数据包，大小为1024字节
    PacketPtr packet = Packet::NewPacket(1024);

    // 创建一个新的RTMP消息头对象
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();

    // 设置消息头的各个字段
    header->cs_id = kRtmpCSIDAMFIni; // 设置控制消息的CSID为AMF初始化
    header->msg_sid = 1;             // 设置消息流ID
    header->msg_len = 0;             // 初始消息长度为0
    header->msg_type = kRtmpMsgTypeAMFMessage; // 设置消息类型为AMF消息

    // 将消息头附加到数据包中
    packet->SetExt(header);

    // 获取数据包的内容区域
    char *body = packet->Data();
    char *p = body;

    // 编码并写入"publish"字符串到消息体
    p += AMFAny::EncodeString(p, "publish");

    // 编码并写入事务ID 5到消息体
    p += AMFAny::EncodeNumber(p, 5);

    // 编码并写入AMF空值（null）到消息体
    *p++ = kAMFNull;

    // 编码并写入流名称（name_）到消息体
    p += AMFAny::EncodeString(p, name_);

    // 编码并写入流类型"live"到消息体
    p += AMFAny::EncodeString(p, "live");

    // 计算并设置消息体的长度
    header->msg_len = p - body;

    // 更新数据包的大小
    packet->SetPacketSize(header->msg_len);

    // 输出日志，记录发送的publish消息的流名称和消息长度
    RTMP_TRACE << " send publish name : " << name_ << " msg_len : " << header->msg_len << " to host : " << connection_->PeerAddr().ToIpPort();
    
    // 将数据包放入输出队列中
    PushOutQueue(std::move(packet));
}

void RtmpContext::HandlePublish(AMFObject &obj)
{
    // 从AMF对象中获取事务ID
    auto tran_id = obj.Property(1)->Number();

    // 从AMF对象中获取流名称
    name_ = obj.Property(3)->String();

    // 输出日志，记录接收到的publish消息的会话名称和参数
    RTMP_TRACE << " recv publish session_name : " << session_name_ << " param : " << param_ << " host : " << connection_->PeerAddr().ToIpPort();

    // 设置标志，表示当前不是播放器
    is_player_ = false;

    // 发送状态消息，表示开始发布流
    SendStatus("status", "NetStream.Publish.Start", "Start Publishing");

    // 如果有RTMP处理器，调用其OnPublish方法，通知开始发布流
    if (rtmp_handler_)
    {
        rtmp_handler_->OnPublish(connection_, session_name_, param_);
    }
}

void RtmpContext::HandleResult(AMFObject &obj)
{
    // 从AMF对象中获取结果ID
    auto id = obj.Property(1)->Number();

    // 输出日志，记录接收到的结果ID
    RTMP_TRACE << " recv result id : " << id << " host : " << connection_->PeerAddr().ToIpPort();

    // 根据结果ID的值执行相应的操作
    if (id == 1)
    {
        // 如果结果ID为1，发送创建流的请求
        SendCreateStream();
    }
    else if (id == 4)
    {
        // 如果结果ID为4，根据当前是否是播放器发送播放或发布的请求
        if (is_player_)
        {
            // 如果当前是播放器，发送播放请求
            SendPlay();
        }
        else 
        {
            // 如果当前不是播放器，发送发布请求
            SendPublish();
        }
    }
}

void RtmpContext::HandleError(AMFObject &obj)
{
    // 从AMF对象中提取错误描述信息
    const std::string &description = obj.Property(3)->Object()->Property("description")->String();

    // 输出日志，记录接收到的错误描述信息
    RTMP_ERROR << " recv error description : " << description << " host : " << connection_->PeerAddr().ToIpPort();
    
    // 关闭与客户端的连接
    connection_->ForceClose();
}