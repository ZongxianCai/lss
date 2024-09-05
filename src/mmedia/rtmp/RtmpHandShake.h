#pragma once
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <string>
#include <memory>
#include <cstdint>
#include "network/net/TcpConnection.h"

namespace lss
{
    using namespace lss::network;

    namespace mm
    {
        // 常量，表示RTMP握手包的大小为1536字节
        const int kRtmpHandShakePacketSize = 1536;

        enum RtmpHandShakeState
        {
            kHandShakeInit,     // 握手初始化状态

            //client
            kHandShakePostC0C1,     // 发送C0和C1阶段
            kHandShakeWaitS0S1,     // 等待接收S0和S1阶段
            kHandShakePostC2,       // 发送C2阶段
            kHandShakeWaitS2,       // 等待接收S2阶段
            kHandShakeDoning,       // 握手进行中状态

            //server
            kHandShakeWaitC0C1,     // 等待接收C0和C1阶段
            kHandShakePostS0S1,     // 发送S0和S1阶段
            kHandShakePostS2,       // 发送S2阶段
            kHandShakeWaitC2,       // 等待接收C2阶段

            kHandShakeDone          // 握手完成状态
        };

        class RtmpHandShake
        {
        public:
            // 构造函数，接受一个TCP连接指针和一个可选的布尔值表示是否为客户端
            RtmpHandShake(const TcpConnectionPtr &conn, bool client = false);

            // 开始握手
            void Start();

            // 处理握手逻辑，接受一个消息缓冲区作为参数
            int32_t HandShake(MsgBuffer &buff);

            // 写操作完成时的回调函数
            void WriteComplete();

            // 默认析构函数
            ~RtmpHandShake() = default;

        private:
            // 生成随机数
            uint8_t GenRandom();

            // 创建C1或S1包
            void CreateC1S1();

            // 检查C1或S1包的正确性
            int32_t CheckC1S1(const char *data, int bytes);

            // 发送C1或S1包
            void SendC1S1();

            /*
             * 创建C2或S2包
             * param data: 表示S1或C1的握手包
             * param bytes: 表示S1或C1的握手包大小
             * param offset: 表示S1或C1的握手包digest的位置
             */
            void CreateC2S2(const char *data, int bytes, int offset);

            // 发送C2或S2包
            void SendC2S2();

            // 检查C2或S2包的正确性
            bool CheckC2S2(const char *data, int bytes);

            // TCP连接指针，用于管理握手过程中使用的TCP连接
            TcpConnectionPtr connection_;

            // 布尔值，标识当前实例是否为客户端
            bool is_client_{false};

            // 布尔值，标识是否进行复杂的握手过程
            bool is_complex_handshake_{true};

            // 用于存储SHA256摘要的数组
            uint8_t digest_[SHA256_DIGEST_LENGTH];

            // 存储C1或S1握手包的数组
            uint8_t C1S1_[kRtmpHandShakePacketSize + 1];

            // 存储C2或S2握手包的数组
            uint8_t C2S2_[kRtmpHandShakePacketSize];

            // 当前握手状态，初始化为kHandShakeInit
            int32_t state_{kHandShakeInit};
        };

        // Rtmp握手包的智能指针
        using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;
    }
}