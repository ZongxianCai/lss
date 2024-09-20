#include "AMFNull.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace lss::mm;

AMFNull::AMFNull(const std::string &name)
    : AMFAny(name)          // 调用父类 AMFAny 的构造函数，初始化对象名称
{

}

AMFNull::AMFNull()
{

}

// 解码函数，参数 data 是输入数据，size 是数据大小，has 表示是否有额外数据
int AMFNull::Decode(const char *data, int size, bool has)
{
    // 返回 0，表示此类型没有实际数据可解码
    return 0;
}

bool AMFNull::IsNull()
{
    // 重写 IsNull 方法，表示当前对象是 null
    return true;
}

void AMFNull::Dump() const
{
    // 打印该对象的详细信息
    RTMP_TRACE << " Null ";
}

AMFNull::~AMFNull()
{

}