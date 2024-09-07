#pragma once
#include "AMFAny.h"

namespace lss
{
    namespace mm
    {
        // AMFDate 类继承自 AMFAny，用于表示 AMF 数据中的日期类型
        class AMFDate : public AMFAny
        {
        public:
            // 构造函数，接受一个字符串参数用于初始化 name_ 成员变量
            AMFDate(const std::string &name);

            // 默认构造函数
            AMFDate();

            // 重写 Decode 函数，用于解码数据，并将解码后的值存储为日期
            int Decode(const char *data, int size, bool has = false) override;

            // 重写 IsDate 函数，判断当前对象是否为日期类型，返回 true
            bool IsDate() override;

            // 重写 Date 函数，返回存储的日期值（UTC 时间戳）
            double Date() override;

            // 重写 Dump 函数，用于输出调试信息，打印存储的日期值和 UTC 偏移量
            void Dump() const override;

            // 返回 UTC 偏移量
            int16_t UtcOffset() const;

            // 析构函数，释放资源
            ~AMFDate();

        private:
            // 存储解码后的 UTC 时间戳
            double utc_{0.0f};

            // 存储解码后的 UTC 偏移量
            int16_t utc_offset_{0};
        };
    }
}