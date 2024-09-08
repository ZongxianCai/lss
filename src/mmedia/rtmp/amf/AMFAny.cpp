#include <cstring>
#include <netinet/in.h>
#include "AMFAny.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "mmedia/base/BytesWriter.h"

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

// -------------------- AMF 封装函数 ---------------------

int AMFAny::WriteNumber(char *buff, double value)
{
    // 定义变量 res 用于存储字节顺序转换后的结果
    uint64_t res;

    // 定义变量 in 用于存储双精度浮点数 value 的二进制表示
    uint64_t in;

    // 将双精度浮点数 value 的二进制表示复制到 in 中
    memcpy(&in, &value, sizeof(double));

    // 将 in 的字节顺序从主机字节序转换为网络字节序，结果存储在 res 中
    res = __bswap_64(in);

    // 将字节顺序转换后的值复制到输出缓冲区 buff 中
    memcpy(buff, &res, 8);

    // 返回写入缓冲区的字节数，这里是 8 字节
    return 8;
}

int32_t AMFAny::EncodeNumber(char *output, double dVal)
{
    // 定义指针 p 指向输出缓冲区 output 的起始位置
    char *p = output;

    // 将 AMF 编码的数字类型标识符 kAMFNumber 写入缓冲区
    *p++ = kAMFNumber;

    // 将双精度浮点数 dVal 编码为 AMF 数字格式并写入缓冲区，更新指针位置
    p += WriteNumber(p, dVal);

    // 返回写入缓冲区的总字节数
    return p - output;
}

int32_t AMFAny::EncodeString(char *output, const std::string& str)
{
    // 定义指针 p 指向输出缓冲区 output 的起始位置
    char *p = output;

    // 获取字符串的长度
    auto len = str.size();

    // 将 AMF 编码的字符串类型标识符 kAMFString 写入缓冲区
    *p++ = kAMFString;

    // 将字符串长度编码为 16 位无符号整数并写入缓冲区，更新指针位置
    p += BytesWriter::WriteUint16T(p, len);

    // 将字符串内容复制到缓冲区中
    memcpy(p, str.c_str(), len);

    // 更新指针位置
    p += len;

    // 返回写入缓冲区的总字节数
    return p - output;
}

int32_t AMFAny::EncodeBoolean(char *output, bool b)
{
    // 定义指针 p 指向输出缓冲区 output 的起始位置
    char *p = output;

    // 将 AMF 编码的布尔类型标识符 kAMFBoolean 写入缓冲区
    *p++ = kAMFBoolean;

    // 根据布尔值 b 的真假，写入 0x01 或 0x00 到缓冲区
    *p++ = b ? 0x01 : 0x00;

    // 返回写入缓冲区的总字节数
    return p - output;
}

int AMFAny::EncodeName(char *buff, const std::string &name)
{
    // 保存缓冲区的初始位置
    char *old = buff;

    // 获取名称字符串的长度
    auto len = name.size();

    // 将名称长度编码为网络字节序的 16 位无符号整数
    unsigned short length = htons(len);

    // 将长度值写入缓冲区
    memcpy(buff, &length, 2);
    
    // 更新缓冲区位置
    buff += 2;

    // 将名称字符串内容复制到缓冲区中
    memcpy(buff, name.c_str(), len);

    // 更新缓冲区位置
    buff += len;

    // 返回写入缓冲区的总字节数（长度字段 2 字节 + 名称内容）
    return len + 2;
}

int32_t AMFAny::EncodeNamedNumber(char *output, const std::string &name, double dVal)
{
    // 保存输出缓冲区的初始位置
    char *old = output;

    // 编码名称并写入缓冲区，更新输出指针
    output += EncodeName(output, name);

    // 编码数字并写入缓冲区，更新输出指针
    output += EncodeNumber(output, dVal);

    // 返回写入缓冲区的总字节数
    return output - old;
}

int32_t AMFAny::EncodeNamedString(char *output, const std::string &name, const std::string &value)
{
    // 保存输出缓冲区的初始位置
    char *old = output;

    // 编码名称并写入缓冲区，更新输出指针
    output += EncodeName(output, name);

    // 编码字符串并写入缓冲区，更新输出指针
    output += EncodeString(output, value);

    // 返回写入缓冲区的总字节数
    return output - old;
}

int32_t AMFAny::EncodeNamedBoolean(char *output, const std::string &name, bool bVal)
{
    // 保存输出缓冲区的初始位置
    char *old = output;

    // 编码名称并写入缓冲区，更新输出指针
    output += EncodeName(output, name);

    // 编码布尔值并写入缓冲区，更新输出指针
    output += EncodeBoolean(output, bVal);
    
    // 返回写入缓冲区的总字节数
    return output - old;
}

AMFAny::~AMFAny()
{

}