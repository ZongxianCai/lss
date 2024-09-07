#include "AMFAny.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace lss::mm;

namespace
{
    // 定义一个静态的空字符串，作为默认返回值，避免返回临时对象
    static std::string empty_string;
}

AMFAny::AMFAny(const std::string &name)
    : name_(name)       // 初始化name_成员变量
{

}

AMFAny::AMFAny()
{

}

const std::string &AMFAny::String()
{
    // 如果当前对象是字符串类型，则返回该字符串
    if (this->IsString())
    {
        return this->String();
    }

    // 如果不是字符串类型，记录错误信息
    RTMP_ERROR << " not a String.";

    // 返回空字符串
    return empty_string;
}

bool AMFAny::Boolean()
{
    // 如果当前对象是布尔类型，则返回该布尔值
    if (this->IsBoolean())
    {
        return this->Boolean();
    }

    // 如果不是布尔类型，记录错误信息
    RTMP_ERROR << " not a Boolean.";

    // 返回false作为默认值
    return false;
}

double AMFAny::Number()
{
    // 如果当前对象是数字类型，则返回该数字
    if (this->IsNumber())
    {
        return this->Number();
    }

    // 如果不是数字类型，记录错误信息
    RTMP_ERROR << " not a Number.";

    // 返回0.0作为默认值
    return 0.0f;
}

double AMFAny::Date()
{
    // 如果当前对象是日期类型，则返回该日期
    if (this->IsDate())
    {
        return this->Date();
    }

    // 如果不是日期类型，记录错误信息
    RTMP_ERROR << " not a Date.";

    // 返回0.0作为默认值
    return 0.0f;
}

AMFObjectPtr AMFAny::Object()
{
    // 如果当前对象是对象类型，则返回该对象
    if (this->IsObject())
    {
        return this->Object();
    }

    // 如果不是对象类型，记录错误信息
    RTMP_ERROR << " not a Object.";

    // 返回一个空的AMFObject智能指针
    return AMFObjectPtr();
}

bool AMFAny::IsString()
{
    // 默认返回false，子类可以重写此方法以返回正确结果
    return false;
}

bool AMFAny::IsNumber()
{
    // 默认返回false，子类可以重写此方法以返回正确结果
    return false;
}

bool AMFAny::IsBoolean()
{
    // 默认返回false，子类可以重写此方法以返回正确结果
    return false;
}

bool AMFAny::IsDate()
{
    // 默认返回false，子类可以重写此方法以返回正确结果
    return false;
}

bool AMFAny::IsObject()
{
    // 默认返回false，子类可以重写此方法以返回正确结果
    return false;
}

const std::string &AMFAny::Name() const
{
    // 返回name_成员变量的值
    return name_;
}

int32_t AMFAny::Count() const
{
    // 默认返回1，表示单一对象
    return 1;
}

std::string AMFAny::DecodeString(const char *data)
{
    // 读取字符串的长度（2字节无符号整数）
    auto len = BytesReader::ReadUint16T(data);

    if (len > 0)
    {
        // 从data指针偏移2个字节开始，读取长度为len的字符串
        std::string str(data + 2, len);

         // 返回解码后的字符串
        return str;
    }
    
    // 如果长度为0，则返回空字符串
    return std::string();
}   

AMFAny::~AMFAny()
{

}