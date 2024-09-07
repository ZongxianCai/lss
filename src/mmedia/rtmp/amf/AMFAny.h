#pragma once
#include <memory>
#include <string>
#include <cstdint>

namespace lss
{
    namespace mm
    {
        // 枚举 AMF 数据类型
        enum AMFDataType
        { 
            kAMFNumber = 0,     // 数字
            kAMFBoolean,        // 布尔
            kAMFString,         // 字符串
            kAMFObject,         // 对象
            kAMFMovieClip,		// 影片剪辑，保留类型，未使用
            kAMFNull,           // 空
            kAMFUndefined,      // 未定义
            kAMFReference,      // 引用
            kAMFEcmaArray,      // ECMAScript 数组
            kAMFObjectEnd,      // 对象结束标志
            kAMStrictArray,     // 严格数组
            kAMFDate,           // 日期
            kAMFLongString,     // 长字符串
            kAMFUnsupported,    // 不支持的数据类型
            kAMFRecordset,		// 记录集，保留类型，未使用
            kAMFXMLDoc,         // XML 文档
            kAMFTypedObject,    // 类型化对象
            kAMFAvmplus,		// AvmPlus对象，用于切换到AMF3
            kAMFInvalid = 0xff  // 无效的AMF数据类型，使用0xff表示无效值
        };

        // 前向声明，防止循环引用
        class AMFObject;

        // 定义 AMFObjectPtr 类型，为 AMFObject 类的智能指针类型，使用 std::shared_ptr 管理内存
        using AMFObjectPtr = std::shared_ptr<AMFObject>;

        // 定义 AMFAny 类，继承 std::enable_shared_from_this，以启用 shared_from_this 功能，使类对象可以安全地生成自身的智能指针
        class AMFAny : public std::enable_shared_from_this<AMFAny>
        {
        public:
            // 构造函数，接受一个字符串参数，用于初始化name_成员变量
            AMFAny(const std::string &name);

            // 默认构造函数
            AMFAny();

            // 纯虚函数，解码AMF数据，子类需要实现，返回值为解码的字节数
            virtual int Decode(const char *data, int size, bool has = false) = 0;

            // 虚函数，返回字符串类型的值
            virtual const std::string &String();

            // 虚函数，返回布尔类型的值
            virtual bool Boolean();

            // 虚函数，返回数字类型的值
            virtual double Number();

            // 虚函数，返回日期类型的值
            virtual double Date();

            // 虚函数，返回对象类型的值
            virtual AMFObjectPtr Object();

            // 虚函数，判断是否为字符串类型
            virtual bool IsString();

            // 虚函数，判断是否为数字类型
            virtual bool IsNumber();

            // 虚函数，判断是否为布尔类型
            virtual bool IsBoolean();

            // 虚函数，判断是否为日期类型
            virtual bool IsDate();

            // 虚函数，判断是否为对象类型
            virtual bool IsObject();

            // 纯虚函数，用于输出调试信息，子类需要实现
            virtual void Dump() const = 0;

            // 返回name_成员变量的值
            const std::string &Name() const;

            // 虚函数，返回子元素的数量，默认为1
            virtual int32_t Count() const;

            // 虚析构函数，确保派生类的析构函数也被调用
            virtual ~AMFAny();

        protected:
            // 静态函数，解码字符串数据，返回解码后的字符串
            static std::string DecodeString(const char *data);
            
            // 成员变量，存储AMF数据的名称
            std::string name_;
        };
    }
}