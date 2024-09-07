#pragma once
#include <vector>
#include "AMFAny.h"

namespace lss
{
    namespace mm
    {
        // 使用智能指针 std::shared_ptr 作为 AMFAny 类的指针类型，方便内存管理
        using AMFAnyPtr = std::shared_ptr<AMFAny>;

        // AMFObject 类继承自 AMFAny，用于表示 AMF 数据中的对象类型
        class AMFObject : public AMFAny
        {
        public:
            // 构造函数，接受一个字符串参数用于初始化 name_ 成员变量
            AMFObject(const std::string &name);

            // 默认构造函数
            AMFObject();

            // 重写 Decode 函数，用于解码数据，并将解码后的值存储为 AMF 对象
            int Decode(const char *data, int size, bool has = false) override;

            // 重写 IsObject 函数，判断当前对象是否为对象类型，返回 true
            bool IsObject() override;

            // 重写 Object 函数，返回当前对象的智能指针
            AMFObjectPtr Object() override;

            // 重写 Dump 函数，用于输出调试信息，打印存储的对象属性
            void Dump() const override;

            // DecodeOnce 函数用于单次解码操作，返回解码消耗的字节数
            int DecodeOnce(const char *data, int size, bool has = false);

            // 根据属性名获取对象中的属性，返回该属性的指针
            const AMFAnyPtr &Property(const std::string &name) const;

            // 根据索引获取对象中的属性，返回该属性的指针
            const AMFAnyPtr &Property(int index) const;

            // 析构函数，释放资源
            ~AMFObject();

        private:
            // 使用 std::vector 容器存储对象的所有属性，每个属性都是 AMFAny 的智能指针
            std::vector<AMFAnyPtr> properties_;
        };
    }
}