#include "AMFNumber.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace lss::mm;

AMFNumber::AMFNumber(const std::string &name)
    : AMFAny(name)          // 初始化 name_ 成员变量
{

}

AMFNumber::AMFNumber()
{

}

int AMFNumber::Decode(const char *data, int size, bool has)
{
    // 如果数据大小不小于8字节，则解码为64位无符号整数，并存储到 number_ 中
    if (size >= 8)
    {
        number_ = BytesReader::ReadUint64T(data);
        // 返回解码的字节数（8字节）
        return 8;
    }

    // 如果数据大小小于8字节，则返回-1，表示解码失败
    return -1;
}

bool AMFNumber::IsNumber()
{
    // 返回 true，表示当前对象是数字类型
    return true;
}

double AMFNumber::Number()
{
    // 返回存储的数字值
    return number_;
}

void AMFNumber::Dump() const
{
    // 输出调试信息，打印存储的数字值
    RTMP_TRACE << " Number : " << number_;
}

AMFNumber::~AMFNumber()
{

}