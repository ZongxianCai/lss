#include "AMFDate.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace lss::mm;

AMFDate::AMFDate(const std::string &name)
    : AMFAny(name)          // 初始化 name_ 成员变量
{

}

AMFDate::AMFDate()
{

}

int AMFDate::Decode(const char *data, int size, bool has)
{
    // 如果数据长度小于10字节，返回-1，表示解码失败
    if (size < 10)
    {
        return -1;
    }

    // 读取数据的前8个字节，解释为 UTC 时间戳
    utc_ = BytesReader::ReadUint64T(data);

    // 将指针偏移8个字节，跳过已读取的 UTC 时间戳
    data += 8;

    // 读取接下来的2个字节，解释为 UTC 偏移量
    utc_offset_ = BytesReader::ReadUint16T(data);

    // 返回解码的总字节数，包括 UTC 时间戳和偏移量
    return 10;
}

bool AMFDate::IsDate()
{
    // 返回 true，表示当前对象是日期类型
    return true;
}

double AMFDate::Date()
{
    // 返回存储的 UTC 时间戳
    return utc_;
}

void AMFDate::Dump() const
{
    // 输出调试信息，打印存储的 UTC 时间戳和 UTC 偏移量
    RTMP_TRACE << " Date : " << utc_ << " , " << utc_offset_;
}

int16_t AMFDate::UtcOffset() const
{
    // 返回存储的 UTC 偏移量
    return utc_offset_;
}

AMFDate::~AMFDate()
{

}