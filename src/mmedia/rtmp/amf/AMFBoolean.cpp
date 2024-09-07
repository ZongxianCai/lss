#include "AMFBoolean.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace lss::mm;

AMFBoolean::AMFBoolean(const std::string &name)
    : AMFAny(name)              // 初始化 name_ 成员变量
{

}

// 默认构造函数
AMFBoolean::AMFBoolean()
{

}

int AMFBoolean::Decode(const char *data, int size, bool has)
{
    // 如果数据大小不小于1字节，则读取该字节并判断其是否为0，存储为布尔值
    if (size >= 1)
    {
        b_ = *data != 0 ? true : false;
        // 返回解码的字节数（1字节）
        return 1;
    }

    // 如果数据大小小于1字节，则返回-1，表示解码失败
    return -1;
}

bool AMFBoolean::IsBoolean()
{
    // 返回 true，表示当前对象是布尔类型
    return true;
}

bool AMFBoolean::Boolean()
{
    // 返回存储的布尔值
    return b_;
}

void AMFBoolean::Dump() const
{
    // 输出调试信息，打印存储的布尔值
    RTMP_TRACE << " Boolean : " << b_;
}

AMFBoolean::~AMFBoolean()
{

}