#pragma once
#include "AMFAny.h"

namespace lss
{
    namespace mm
    {
        // AMFLongString 类继承自 AMFAny，用于表示 AMF 数据中的长字符串类型
        class AMFLongString : public AMFAny
        {
        public:
            // 构造函数，接受一个字符串参数用于初始化 name_ 成员变量
            AMFLongString(const std::string &name);

            // 默认构造函数
            AMFLongString();

            // 重写 Decode 函数，用于解码数据，并将解码后的值存储为长字符串
            int Decode(const char *data, int size, bool has = false) override;

            // 重写 IsString 函数，判断当前对象是否为字符串类型，返回 true
            bool IsString() override;

            // 重写 String 函数，返回存储的字符串值
            const std::string& String() override;

            // 重写 Dump 函数，用于输出调试信息，打印存储的字符串值
            void Dump() const override;

            // 析构函数，释放资源
            ~AMFLongString();
            
        private:
            // 存储解码后的字符串值
            std::string string_;
        };
    }
}