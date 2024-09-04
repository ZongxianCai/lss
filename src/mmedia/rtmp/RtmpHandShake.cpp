#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <cstdint>
#include <random>
#include "RtmpHandShake.h"
#include "base/TTime.h"
#include "mmedia/base/MMediaHandler.h"
#include "mmedia/base/MMediaLog.h"

// 根据 OpenSSL 版本定义不同的 HMAC 初始化、更新和完成宏
#if OPENSSL_VERSION_NUMBER > 0x10100000L
#define HMAC_setup(ctx, key, len)	ctx = HMAC_CTX_new(); HMAC_Init_ex(ctx, key, len, EVP_sha256(), 0)
#define HMAC_crunch(ctx, buf, len)	HMAC_Update(ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen)	HMAC_Final(ctx, dig, &dlen); HMAC_CTX_free(ctx)
#else
#define HMAC_setup(ctx, key, len)	HMAC_CTX_init(&ctx); HMAC_Init_ex(&ctx, key, len, EVP_sha256(), 0)
#define HMAC_crunch(ctx, buf, len)	HMAC_Update(&ctx, buf, len)
#define HMAC_finish(ctx, dig, dlen)	HMAC_Final(&ctx, dig, &dlen); HMAC_CTX_cleanup(&ctx)
#endif

namespace
{
    // 定义 RTMP 服务器版本号
    static const unsigned char rtmp_server_ver[4] = 
    {
        0x0D, 0x0E, 0x0A, 0x0D
    };

    // 定义 RTMP 客户端版本号
    static const unsigned char rtmp_client_ver[4] = 
    {
        0x0C, 0x00, 0x0D, 0x0E
    };

    // 定义客户端密钥部分长度
    #define PLAYER_KEY_OPEN_PART_LEN 30   ///< length of partial key used for first client digest signing
    
    // 客户端密钥，用于生成 HMAC 签名
    static const uint8_t rtmp_player_key[] = {
        'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
        'F', 'l', 'a', 's', 'h', ' ', 'P', 'l', 'a', 'y', 'e', 'r', ' ', '0', '0', '1',

        0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02,
        0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8,
        0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
    };

    // 定义服务器密钥部分长度
    #define SERVER_KEY_OPEN_PART_LEN 36   ///< length of partial key used for first server digest signing
    
    // 服务器密钥，用于生成 HMAC 签名
    static const uint8_t rtmp_server_key[] = {
        'G', 'e', 'n', 'u', 'i', 'n', 'e', ' ', 'A', 'd', 'o', 'b', 'e', ' ',
        'F', 'l', 'a', 's', 'h', ' ', 'M', 'e', 'd', 'i', 'a', ' ',
        'S', 'e', 'r', 'v', 'e', 'r', ' ', '0', '0', '1',

        0xF0, 0xEE, 0xC2, 0x4A, 0x80, 0x68, 0xBE, 0xE8, 0x2E, 0x00, 0xD0, 0xD1, 0x02,
        0x9E, 0x7E, 0x57, 0x6E, 0xEC, 0x5D, 0x2D, 0x29, 0x80, 0x6F, 0xAB, 0x93, 0xB8,
        0xE6, 0x36, 0xCF, 0xEB, 0x31, 0xAE
    };

    // 计算消息的 HMAC 签名
    void CalculateDigest(const uint8_t *src, int len, int gap, const uint8_t *key, int keylen, uint8_t *dst)
    {
        // 存储 HMAC 签名的长度
        uint32_t digestLen = 0;

        // 根据 OpenSSL 版本定义上下文
        #if OPENSSL_VERSION_NUMBER > 0x10100000L    
        HMAC_CTX *ctx;      // OpenSSL 1.1.0 及以上版本使用指针
        #else
        HMAC_CTX ctx;       // OpenSSL 1.1.0 以下版本使用实例
        #endif

        // 初始化 HMAC 上下文并设置密钥
        HMAC_setup(ctx, key, keylen);
        
        // 如果 gap <= 0，说明没有间隔，直接处理全部数据
        if (gap <= 0)
        {
            /// 计算整个消息的 HMAC
            HMAC_crunch(ctx, src, len);
        }        
        else
        {   
            // 如果有间隔，先处理间隔之前的数据                  
            HMAC_crunch(ctx, src, gap);
            // 跳过间隔并处理剩余的数据
            HMAC_crunch(ctx, src + gap + SHA256_DIGEST_LENGTH, len - gap - SHA256_DIGEST_LENGTH);
        }

        // 计算 HMAC 结果并释放上下文
        HMAC_finish(ctx, dst, digestLen);
    }

    // 验证消息的 HMAC 签名是否正确
    bool VerifyDigest(uint8_t *buff, int digest_pos, const uint8_t *key, size_t keyLen)
    {
        // 用于存储计算出来的 HMAC 签名
        uint8_t digest[SHA256_DIGEST_LENGTH];
        // 计算消息的 HMAC 签名
        CalculateDigest(buff, 1536, digest_pos, key, keyLen, digest);

        // 比较计算出的 HMAC 与消息中的 HMAC 是否一致，若一致则返回 true
        return memcmp(&buff[digest_pos], digest, SHA256_DIGEST_LENGTH) == 0;
    }

    // 计算消息中某个位置的偏移量，用于寻找签名的位置
    int32_t GetDigestOffset(const uint8_t *buff, int off, int mod_val)
    {
        // 用于存储偏移量
        uint32_t offset = 0;
        // 读取指定位置的数据指针
        const uint8_t *ptr = reinterpret_cast<const uint8_t *>(buff + off);
        // 计算出的偏移量
        uint32_t res;

        // 通过四个字节的数据计算出偏移量
        offset = ptr[0] + ptr[1] + ptr[2] + ptr[3];
        // 将偏移量按指定值取模，然后加上基准偏移量
        res = (offset % mod_val) + (off + 4);

        // 返回最终计算出的偏移量
        return res;
    }
}

using namespace lss::mm;

// RtmpHandShake 类的构造函数，初始化连接和是否为客户端
RtmpHandShake::RtmpHandShake(const TcpConnectionPtr &conn, bool client)
    : connection_(conn)         // 初始化连接对象
    , is_client_(client)        // 初始化客户端标志
{

}

// 开始 RTMP 握手
void RtmpHandShake::Start()
{
    // 生成 C1 或 S1 数据
    CreateC1S1();

    // 如果是客户端，设置握手状态为 kHandShakePostC0C1，并发送 C1S1 数据
    if (is_client_)
    {
        state_ = kHandShakePostC0C1;

        SendC1S1();
    }
    else    // 如果是服务端，设置握手状态为 kHandShakeWaitC0C1，等待客户端数据
    {
        state_ = kHandShakeWaitC0C1;
    }
}

// 生成随机字节
uint8_t RtmpHandShake::GenRandom()
{
    // 使用随机设备生成随机数种子
    std::mt19937 mt{std::random_device{}()};
     // 在 0 到 256 之间生成均匀分布的随机数
    std::uniform_int_distribution<> rand(0, 256);

    // 返回随机数
    return rand(mt) % 256;
}

void RtmpHandShake::CreateC1S1()
{
    // 为 C1S1 数据包中的每一个字节生成随机值
    for (int i = 0; i < kRtmpHandShakePacketSize + 1; i++)
    {
        C1S1_[i] = GenRandom();
    }

    // 设置 RTMP 协议版本为 3
    C1S1_[0] = '\x03';

    // 设置时间戳部分为 0
    memset(C1S1_ + 1, 0x00, 4);

    // 如果不是复杂握手，设置版本部分为 0
    if (!is_complex_handshake_)
    {
        memset(C1S1_ + 5, 0x00, 4);
    }
    else    // 如果是复杂握手
    {
        // 计算消息中 HMAC 的偏移位置
        auto offset = GetDigestOffset(C1S1_ + 1, 8, 728);
        // 获取偏移位置的指针
        uint8_t * data = C1S1_ + 1 + offset;

        // 如果是客户端，使用客户端的版本信息和密钥进行签名计算
        if (is_client_)
        {
            // 设置客户端版本
            memcpy(C1S1_ + 5, rtmp_client_ver, 4);

            // 使用客户端密钥计算 HMAC 签名
            CalculateDigest(C1S1_ + 1, kRtmpHandShakePacketSize, offset, rtmp_player_key, PLAYER_KEY_OPEN_PART_LEN, data);
        }
        else    // 如果是服务端，使用服务端的版本信息和密钥进行签名计算
        {
            // 设置服务端版本
            memcpy(C1S1_ + 5, rtmp_server_ver, 4);
            // 使用服务端密钥计算 HMAC 签名
            CalculateDigest(C1S1_ + 1, kRtmpHandShakePacketSize, offset, rtmp_server_key, SERVER_KEY_OPEN_PART_LEN, data);            
        }

        // 将计算出来的签名存储到 digest_ 中
        memcpy(digest_, data, SHA256_DIGEST_LENGTH);
    }
}

int32_t RtmpHandShake::CheckC1S1(const char *data, int bytes)
{
    // 检查数据包的长度是否为 1537 字节，不符合则返回错误
    if (bytes != 1537)
    {
        // 记录错误日志
        RTMP_ERROR << " unexpected C1S1, len = " << bytes;

        // 返回错误码
        return -1;
    }

    // 检查数据包的第一个字节是否为 '\x03'，即 RTMP 协议版本，不符合则返回错误
    if (data[0] != '\x03')
    {
        // 记录错误日志
        RTMP_ERROR << " unexpected C1S1, ver = " << data[0];

        // 返回错误码
        return -1;
    }

    // 读取第 5 到第 8 字节的版本信息
    uint32_t *version = (uint32_t*) (data + 5);

    // 如果版本为 0，说明是简单握手，返回 0 表示正常
    if (*version == 0)
    {
        // 标记为简单握手
        is_complex_handshake_ = false;

        // 返回 0 表示简单握手
        return 0;
    }

    // 用于存储 HMAC 偏移量的变量
    int32_t offset = -1;

    // 如果是复杂握手
    if (is_complex_handshake_)
    {
        // 指向握手数据的指针
        uint8_t *handshake = (uint8_t *)(data + 1);
        // 获取第一个 HMAC 偏移量
        offset = GetDigestOffset(handshake, 8, 728);

        // 如果是客户端
        if (is_client_)
        {
            // 验证 HMAC 签名，使用服务端密钥
            if (!VerifyDigest(handshake, offset, rtmp_server_key, SERVER_KEY_OPEN_PART_LEN))
            {
                // 如果验证失败，尝试使用第二个偏移量进行验证
                offset = GetDigestOffset(handshake, 772, 728);

                // 如果验证仍然失败，返回错误码
                if (!VerifyDigest(handshake, offset, rtmp_server_key, SERVER_KEY_OPEN_PART_LEN))
                {
                    return -1;
                }
            }
        }
        else    // 如果是服务端
        {
            // 验证 HMAC 签名，使用客户端密钥
            if (!VerifyDigest(handshake, offset, rtmp_player_key, PLAYER_KEY_OPEN_PART_LEN))
            {
                // 如果验证失败，尝试使用第二个偏移量进行验证
                offset = GetDigestOffset(handshake, 772, 728);

                // 如果验证仍然失败，返回错误码
                if (!VerifyDigest(handshake, offset, rtmp_player_key, PLAYER_KEY_OPEN_PART_LEN))
                {
                    return -1;
                }
            }
        }
    }

    // 返回计算出的偏移量，若为简单握手则返回 0
    return offset;
}

void RtmpHandShake::SendC1S1()
{
    // 使用连接对象发送 C1 或 S1 数据包，大小为 1537 字节
    connection_->Send((const char *)C1S1_, 1537);
}

void RtmpHandShake::CreateC2S2(const char *data, int bytes, int offset)
{
    // 为 C2S2 数据包中的每一个字节生成随机值
    for (int i = 0; i < kRtmpHandShakePacketSize; i++)
    {
        C2S2_[i] = GenRandom();
    }

    // 将前 8 字节的数据从接收到的数据中复制到 C2S2 中
    memcpy(C2S2_, data, 8);

    // 获取当前时间戳
    auto timestamp = lss::base::TTime::Now();

    // 将时间戳的高字节放在前面，低字节放在后面
    char *t = (char*) &timestamp;
    C2S2_[3] = t[0];
    C2S2_[2] = t[1];
    C2S2_[1] = t[2];
    C2S2_[0] = t[3];

    // 如果是复杂握手
    if (is_complex_handshake_)
    {
        // 用于存储计算出的 HMAC 签名
        uint8_t digest[32];

        // 如果是客户端，使用客户端密钥进行 HMAC 计算
        if (is_client_)
        {
            CalculateDigest(digest_, 32, 0, rtmp_player_key, sizeof(rtmp_player_key), digest);
        }
        else    // 如果是服务端，使用服务端密钥进行 HMAC 计算
        {
            CalculateDigest(digest_, 32, 0, rtmp_server_key, sizeof(rtmp_server_key), digest);
        }

        // 计算 C2S2 数据包的 HMAC 签名，并将结果填充到数据包末尾
        CalculateDigest(C2S2_, kRtmpHandShakePacketSize - 32, 0, digest, 32, &C2S2_[kRtmpHandShakePacketSize - 32]);
    }

}


void RtmpHandShake::SendC2S2()
{
    // 使用连接对象发送 C2 或 S2 数据包，大小为 1536 字节
    connection_->Send((const char *)C2S2_, kRtmpHandShakePacketSize);
}

bool RtmpHandShake::CheckC2S2(const char *data, int bytes)
{
    // 暂时只返回 true，表示检查通过，实际应用中可能需要进一步验证
    return true;
}

int32_t RtmpHandShake::HandShake(MsgBuffer &buff)
{
    // 根据当前的握手状态进行处理
    switch (state_)
    {
        // 等待接收 C0C1 数据包
        case kHandShakeWaitC0C1:
        {
            // 如果缓冲区中可读字节数少于 1537 字节，则表示数据包未完全到达，返回 1 表示继续等待
            if (buff.ReadableBytes() < 1537)
            {
                return 1;
            }

            // 打印日志，记录接收到 C0C1 数据包的主机信息
            RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , recv C0C1.\n";

            // 检查 C1S1 数据包是否有效，返回偏移量
            auto offset = CheckC1S1(buff.Peek(), 1537);

            // 如果偏移量合法，表示数据包有效
            if (offset >= 0)
            {
                // 创建 S2 数据包并发送
                CreateC2S2(buff.Peek() + 1, 1536, offset);
                // 从缓冲区中移除已经处理的 C0C1 数据
                buff.Retrieve(1537);
                // 更新状态为等待发送 S0S1
                state_ = kHandShakePostS0S1;
                // 发送 C1S1 数据包
                SendC1S1();
            }
            else 
            {
                // 如果数据包检查失败，打印日志并返回错误码 -1
                RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , check C0C1 failed.\n";

                return -1;
            }

            break;
        }

        case kHandShakePostC2:
        {
            // 如果缓冲区中可读字节数少于 1536 字节，表示数据包未完全到达，返回 1 表示继续等待
            if (buff.ReadableBytes() < 1536)
            {
                return 1;
            }

            // 打印日志，记录接收到 C2 数据包的主机信息
            RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , recv C2.\n";

            // 检查 C2 数据包是否有效
            if (CheckC2S2(buff.Peek(), 1536))
            {
                // 从缓冲区中移除已处理的 C2 数据包
                buff.Retrieve(1536);
                RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , handshake done.\n";
                // 更新状态为握手完成
                state_ = kHandShakeDone;
            } 
            else 
            {
                // 如果 C2 数据包检查失败，打印日志并返回错误码 -1
                RTMP_TRACE << "host:" << connection_->PeerAddr().ToIpPort() << " , check C2 failed.\n";

                return -1;
            }

            break;
        }

        // 等待接收 S0S1 数据包的状态
        case kHandShakeWaitS0S1:
        {
            // 如果缓冲区中可读字节数少于 1537 字节，表示数据包未完全到达，返回 1 表示继续等待
            if (buff.ReadableBytes() < 1537)
            {
                return 1;
            }

            // 打印日志，记录接收到 S0S1 数据包的主机信息
            RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , recv S0S1.\n";

            // 检查 S0S1 数据包是否有效，返回偏移量
            auto offset = CheckC1S1(buff.Peek(), 1537);

            // 如果偏移量合法，表示数据包有效
            if (offset >= 0)
            {
                // 创建 S2 数据包并发送
                CreateC2S2(buff.Peek() + 1, 1536, offset);
                // 从缓冲区中移除已处理的 S0S1 数据包
                buff.Retrieve(1537);

                // 如果缓冲区中正好还有 1536 字节的数据，表示 S2 数据包也已经接收完毕
                if (buff.ReadableBytes() == 1536) //S2
                {
                    // 更新状态为握手进行中
                    state_ = kHandShakeDoning;
                    // 发送 C2S2 数据包
                    SendC2S2();

                    // 返回 2 表示握手成功
                    return 2;
                }
                else 
                {
                    // 否则更新状态为等待接收 C2 数据包
                    state_ = kHandShakePostC2;
                    // 发送 C2S2 数据包
                    SendC2S2();
                }
            }
            else 
            {
                // 如果数据包检查失败，打印日志并返回错误码 -1
                RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , check S0S1 failed.\n";

                return -1;
            }

            break;
        }
    }

    // 返回 1 表示继续等待更多数据
    return 1;
}

void RtmpHandShake::WriteComplete()
{
    // 根据当前握手状态进行处理
    switch(state_)
    {
        // 处理 S0S1 数据包发送完成的情况
        case kHandShakePostS0S1:
        {
            // 打印日志，记录发送完成
            RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , post S0S1.\n";
            // 更新状态为等待发送 S2 数据包
            state_ = kHandShakePostS2;
            // 发送 C2S2 数据包
            SendC2S2();

            break;
        }

        // 处理 S2 数据包发送完成的情况
        case kHandShakePostS2:
        {
            // 打印日志，记录发送完成
            RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , post S2.\n";
            // 更新状态为等待接收 C2 数据包
            state_ = kHandShakePostC2;

            break;
        }

        // 处理 C0C1 数据包发送完成的情况
        case kHandShakePostC0C1:
        {
            // 打印日志，记录发送完成
            RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , post C0C1.\n";

            // 更新状态为等待接收 S0S1 数据包
            state_ = kHandShakeWaitS0S1;

            break;
        }

        // 处理 C2 数据包发送完成的情况
        case kHandShakePostC2:
        {
            // 打印日志，记录发送完成
            RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , post C2 done.\n";
            // 更新状态为握手完成
            state_ = kHandShakeDone;

            break;
        }

        // 处理握手进行中的状态
        case kHandShakeDoning:
        {
            // 打印日志，记录握手完成
            RTMP_TRACE << " host : " << connection_->PeerAddr().ToIpPort() << " , post C2 done.\n";
            // 更新状态为握手完成
            state_ = kHandShakeDone;
            
            break;
        }
    }
}