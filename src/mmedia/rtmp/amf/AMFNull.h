#pragma once
#include "AMFAny.h"

namespace lss
{
    namespace mm
    {
        class AMFNull : public AMFAny
        {
        public:
            // 带参数的构造函数，用于根据名称初始化 AMFNull 对象
            AMFNull(const std::string &name);

            // 默认构造函数
            AMFNull();

            // 解码函数，从给定的数据中解析信息，并返回解析结果
            int Decode(const char *data, int size, bool has = false) override;

            // 重写父类方法，返回该对象是否为 null
            bool IsNull() override;

            // 重写父类方法，打印该对象的详细信息
            void Dump() const override;

            // 析构函数，用于清理资源
            ~AMFNull();

        private:
            // 私有成员变量，用于标识是否为 null，默认初始化为 false
            bool b_{false};
        };
    }
}