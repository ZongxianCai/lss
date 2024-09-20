#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include "mmedia/base/Packet.h"

namespace lss
{
    namespace live
    {
        using namespace lss::mm;

        // 定义结构体 GopItemInfo
        struct GopItemInfo
        {
            // GOP 项的索引
            int32_t index;

            // GOP 项的时间戳
            int64_t timestamp;

            // 构造函数，初始化索引和时间戳
            GopItemInfo(int32_t i, int64_t t)
                : index(i), timestamp(t)
            {

            }
        };

        // 定义类 GopMgr
        class GopMgr
        {
        public:
            // 默认构造函数
            GopMgr() = default;

            // 添加帧
            void AddFrame(const PacketPtr &packet);

            // 获取最大 GOP 长度
            int32_t MaxGopLength() const;

            // 获取当前 GOP 数量
            size_t GopSize() const;

            // 根据延迟获取 GOP 
            int GetGopByLatency(int content_latency, int &latency) const;

            // 清除过期 GOP 
            void ClearExpriedGop(int min_idx);

            // 打印所有 GOP 
            void PrintAllGop();

            // 获取最新时间戳
            int64_t LastestTimeStamp() const
            {
                // 返回最新时间戳
                return lastest_timestamp_;
            }

            // 析构函数
            ~GopMgr(){};

        private:
            // 存储 GOP 项的 vector
            std::vector<GopItemInfo> gops_;

            // 当前 GOP 长度
            int32_t gop_length_{0};

            // 最大 GOP 长度
            int32_t max_gop_length_{0};

            // GOP 项的数量
            int32_t gop_numbers_{0};

            // 总 GOP 长度
            int32_t total_gop_length_{0};

            // 最新时间戳
            int64_t lastest_timestamp_{0};
        };

    }
}