#include <cstdint>
#include <netinet/in.h>
#include <cstring>
#include "BytesReader.h"

using namespace lss::mm;

// 读取8字节的无符号整数，并将其从网络字节序转换为主机字节序
uint64_t BytesReader::ReadUint64T(const char *data)
{
    // 将字符指针转换为 64 位无符号整数指针
    uint64_t in  = *((uint64_t*)data);

    // 使用 __bswap_64 函数将网络字节序转换为主机字节序
    uint64_t res = __bswap_64(in);

    double value;

    // 将主机字节序转换为双精度浮点数
    memcpy(&value, &res, sizeof(double));

    // 将双精度浮点数转换为 64 位无符号整数
    return value;
}

// 读取4字节的无符号整数，并将其从网络字节序转换为主机字节序
uint32_t BytesReader::ReadUint32T(const char *data)
{
    // 将字符指针转换为 32 位无符号整数指针
    uint32_t *c = (uint32_t *) data;

    // 使用 ntohl 函数将网络字节序转换为主机字节序
    return ntohl(*c);
}

// 读取3字节的无符号整数
uint32_t BytesReader::ReadUint24T(const char *data)
{
    // 将字符指针转换为 unsigned char 指针
    unsigned char *c = (unsigned char *) data;

    uint32_t val;
    // 通过位移操作从3字节数据中提取整数值
    val = (c[0] << 16) | (c[1] << 8) | c[2];

    return val;
}

// 读取2字节的无符号整数，并将其从网络字节序转换为主机字节序
uint16_t BytesReader::ReadUint16T(const char *data)
{
    // 将字符指针转换为 16 位无符号整数指针
    uint16_t *c = (uint16_t *) data;

    // 使用 ntohs 函数将网络字节序转换为主机字节序
    return ntohs(*c);
}

// 读取1字节的无符号整数
uint8_t BytesReader::ReadUint8T(const char *data)
{
    // 直接返回数据的第一个字节
    return data[0];
}
