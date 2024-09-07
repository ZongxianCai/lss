#include "AMFLongString.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace lss::mm;

AMFLongString::AMFLongString(const std::string &name)
    : AMFAny(name)          // 初始化 name_ 成员变量
{

}

AMFLongString::AMFLongString()
{

}

int AMFLongString::Decode(const char *data, int size, bool has)
{
    // 如果数据长度小于4字节，返回-1，表示解码失败
    if (size < 4)
    {
        return -1;
    }

    // 读取数据的前4个字节，解释为字符串的长度
    auto len = BytesReader::ReadUint32T(data);

    // 如果字符串长度无效或数据总长度不足以容纳字符串内容，返回-1
    if (len < 0 || size < len + 4)
    {
        return -1;
    }

    // 从数据中提取字符串内容，并存储到 string_ 中
    string_.assign(data + 4, len);

    // 返回解码的总字节数，包括字符串长度和内容
    return len + 4;
}

bool AMFLongString::IsString()
{
    // 返回 true，表示当前对象是字符串类型
    return true;
}

const std::string& AMFLongString::String()
{
    // 返回存储的字符串值
    return string_;
}

void AMFLongString::Dump() const
{
    // 输出调试信息，打印存储的长字符串值
    RTMP_TRACE << " LongString : " << string_;
}

AMFLongString::~AMFLongString()
{

}