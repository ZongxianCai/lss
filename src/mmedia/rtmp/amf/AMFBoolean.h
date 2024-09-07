#pragma once
#include "AMFAny.h"

namespace lss
{
    namespace mm
    {
        // AMFBoolean 类继承自 AMFAny，用于表示 AMF 数据中的布尔类型
        class AMFBoolean : public AMFAny
        {
        public:
            // 构造函数，接受一个字符串参数用于初始化 name_ 成员变量
            AMFBoolean(const std::string &name);

            // 默认构造函数
            AMFBoolean();
            
            // 重写 Decode 函数，用于解码数据，并将解码后的值存储为布尔类型
            int Decode(const char *data, int size, bool has = false) override;

            // 重写 IsBoolean 函数，判断当前对象是否为布尔类型，返回 true
            bool IsBoolean() override;

            // 重写 Boolean 函数，返回存储的布尔值
            bool Boolean() override;

            // 重写 Dump 函数，用于输出调试信息，打印存储的布尔值
            void Dump() const override;

            // 析构函数，释放资源
            ~AMFBoolean();

        private:
            // 存储解码后的布尔值，默认为 false
            bool b_{false};
        };
    }
}