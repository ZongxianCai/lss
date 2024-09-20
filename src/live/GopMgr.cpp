#include "GopMgr.h"
#include "live/base/LiveLog.h"

using namespace lss::live;

void GopMgr::AddFrame(const PacketPtr &packet)
{
    // 更新最新时间戳为当前数据包的时间戳
    lastest_timestamp_ = packet->TimeStamp();

    // 如果当前数据包是关键帧
    if (packet->IsKeyFrame())
    {
        // 将关键帧的索引和时间戳添加到 gops_ 向量中
        gops_.emplace_back(packet->Index(), packet->TimeStamp());

        // 更新最大 GOP 长度
        max_gop_length_ = std::max(max_gop_length_, gop_length_);

        // 累加当前 GOP 的长度到总长度
        total_gop_length_ += gop_length_;

        // GOP 数量加一
        gop_numbers_ ++;

        // 重置当前 GOP 长度
        gop_length_ = 0;
    }

    // 当前 GOP 长度加一
    gop_length_++;
}

int32_t GopMgr::MaxGopLength() const
{
    // 返回最大 GOP 长度
    return max_gop_length_;
}

size_t GopMgr::GopSize() const
{
    // 返回 gops_ 向量的大小
    return gops_.size();
}

int GopMgr::GetGopByLatency(int content_latency, int &latency) const
{
    // 初始化获取的 GOP 索引为 -1
    int got = -1;

    // 初始化延迟为 0
    latency = 0;

    // 从 gops_ 向量的末尾开始迭代
    auto iter = gops_.rbegin();

    // 遍历 gops_ 向量
    for (; iter != gops_.rend(); ++iter)
    {
        // 计算当前 GOP 项的延迟
        int item_latency = lastest_timestamp_ - iter->timestamp;

        // 如果当前 GOP 项的延迟在内容延迟范围内
        if (item_latency <= content_latency)
        {
            // 记录 GOP 索引
            got = iter->index;

            // 更新延迟值
            latency = item_latency;
        }
        else 
        {
            // 如果延迟超过范围，结束循环
            break;
        }
    }

    // 返回找到的 GOP 索引
    return got;
}

void GopMgr::ClearExpriedGop(int min_idx)
{
    // 如果 gops_ 向量为空
    if (gops_.empty())
    {
        // 直接返回
        return;
    }

    // 遍历 gops_ 向量
    for (auto iter = gops_.begin(); iter != gops_.end();)
    {
        // 如果当前 GOP 项的索引小于等于最小索引
        if (iter->index <= min_idx)
        {
            // 删除当前 GOP 项，并更新迭代器
            iter = gops_.erase(iter);
        }
        else 
        {
            // 否则，移动到下一个 GOP 项
            iter++;
        }
    }
}

void GopMgr::PrintAllGop()
{
    // 创建一个字符串流对象
    std::stringstream ss;

    // 添加标题到字符串流
    ss << "All gop : ";

    // 遍历 gops_ 向量
    for (auto iter = gops_.begin(); iter != gops_.end(); iter++)
    {
        // 将每个 GOP 项的索引和时间戳添加到字符串流
        ss << "[" << iter->index << ", " << iter->timestamp << "]";
    }

    // 将字符串流的内容输出到日志中
    LIVE_TRACE << ss.str() << "\n";
}