#pragma once
#include <string>
#include <memory>
#include <cstring>
#include <cstdint>

namespace lss
{
    namespace mm
    {
        // 定义一组枚举值，用于表示不同类型的包和帧
        enum
        {
            kPacketTypeVideo = 1,           // 视频包类型
            kPacketTypeAudio = 2,           // 音频包类型
            kPacketTypeMeta = 4,            // 元数据包类型
            kPacketTypeMeta3 = 8,           // 元数据3包类型
            kFrameTypeKeyFrame = 16,        // 关键帧类型
            kFrameTypeIDR = 32,             // IDR帧类型
            kPacketTypeUnknowed = 255       // 未知包类型
        };

        // 前向声明 Packet 类
        class Packet;

        // 定义 Packet 类的智能指针类型别名
        using PacketPtr = std::shared_ptr<Packet>;

    #pragma pack(push)
    #pragma pack(1)     // 设置结构体内存对齐为 1 字节

    // 数据包类
    class Packet
    {
    public:
        // 构造函数，初始化包的容量大小
        Packet(int32_t size)
            : capacity_(size)
        {
        }

        // 静态函数，用于创建新的 Packet 对象
        static PacketPtr NewPacket(int32_t size);

        // 判断包是否为视频类型
        bool IsVideo() const
        {
            return (type_ & kPacketTypeVideo) == kPacketTypeVideo;
        }

        // 判断包是否为关键帧
        bool IsKeyFrame() const
        {
            return ((type_ & kPacketTypeVideo) == kPacketTypeVideo) && ((type_ & kFrameTypeKeyFrame) == kFrameTypeKeyFrame);
        }

        // 判断包是否为音频类型
        bool IsAudio() const
        {
            return type_ == kPacketTypeAudio;
        }

        // 判断包是否为元数据类型
        bool IsMeta() const
        {
            return type_ == kPacketTypeMeta;
        }

        // 判断包是否为元数据3类型
        bool IsMeta3() const
        {
            return type_ == kPacketTypeMeta3;
        }

        // 获取包的大小
        inline int32_t PacketSize() const
        {
            return size_;
        }

        // 获取包的剩余容量空间
        inline int Space() const
        {
            return capacity_ - size_;
        }

        // 设置包的大小
        inline void SetPacketSize(size_t len)
        {
            size_ = len;
        }

        // 更新包的大小，增加 len
        inline  void UpdatePacketSize(size_t len)
        {
            size_ += len;
        }

        // 设置包的索引值
        void SetIndex(int32_t index)
        {
            index_ = index;
        }

        // 获取包的索引值
        int32_t Index() const
        {
            return index_;
        }

        // 设置包的类型
        void SetPacketType(int32_t type)
        {
            type_ = type;
        }

        // 获取包的类型
        int32_t PacketType() const
        {
            return type_;
        }

        // 设置时间戳
        void SetTimestamp(uint64_t timestamp)
        {
            timestamp_ = timestamp;
        }

        // 获取时间戳
        uint64_t Timestamp() const
        {
            return timestamp_;
        }

        // 获取包数据的起始位置
        inline char *Data()
        {
            return (char*)this + sizeof(Packet);
        }

        // 获取扩展数据，使用模板函数和类型安全的智能指针转换
        template <typename T>
        inline std::shared_ptr<T> Ext() const
        {
            return std::static_pointer_cast<T>(ext_);
        }

        // 设置扩展数据
        inline void SetExt(const std::shared_ptr<void>& ext)
        {
            ext_ = ext;
        }

        // 析构函数
        ~Packet() {}

    private:
        // 包的类型，默认为未知类型
        int32_t type_{kPacketTypeUnknowed};

        // 包的大小
        uint32_t size_{0};

        // 包的索引值，默认值为 -1
        int32_t index_{-1};

        // 时间戳
        uint64_t timestamp_{0};

        // 包的容量
        uint32_t capacity_{0};

        // 扩展数据指针，允许存储额外的信息
        std::shared_ptr<void> ext_;
    };

    #pragma pack()     // 恢复默认的内存对齐
    }
}