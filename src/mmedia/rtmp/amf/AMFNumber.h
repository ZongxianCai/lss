#pragma once
#include "AMFAny.h"

namespace lss
{
    namespace mm
    {
        // AMFNumber 类继承自 AMFAny，用于表示 AMF 数据中的数字类型
        class AMFNumber : public AMFAny
        {
        public:
            // 构造函数，接受一个字符串参数用于初始化 name_ 成员变量
            AMFNumber(const std::string &name);

            // 默认构造函数
            AMFNumber();
            
            // 重写 Decode 函数，用于解码数据，并将解码后的值存储为数字
            int Decode(const char *data, int size, bool has = false) override;

            // 重写 IsNumber 函数，判断当前对象是否为数字类型，返回 true
            bool IsNumber() override;

            // 重写 Number 函数，返回存储的数字值
            double Number() override;

            // 重写 Dump 函数，用于输出调试信息，打印存储的数字值
            void Dump() const override;

            // 析构函数，释放资源
            ~AMFNumber();
            
        private:
            // 存储解码后的数字值，默认为0.0
            double number_{0.0f};
        };
    }
}