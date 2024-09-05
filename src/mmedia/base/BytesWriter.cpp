#include <cstring>
#include <arpa/inet.h>
#include "BytesWriter.h"

using namespace lss::mm;

// 将4字节的无符号整数转换为网络字节序并写入缓冲区
int BytesWriter::WriteUint32T(char *buff, uint32_t val)
{
    // 将主机字节序转换为网络字节序
    val = htonl(val);
    // 将转换后的值写入缓冲区
    memcpy(buff, &val, sizeof(int32_t));

    // 返回写入的字节数
    return sizeof(int32_t);
}

// 将3字节的无符号整数转换为网络字节序并写入缓冲区
int BytesWriter::WriteUint24T(char *buff, uint32_t val)
{
    // 将主机字节序转换为网络字节序
    val = htonl(val);
    // 获取转换后值的指针
    char *ptr = (char *)&val;
    // 跳过第一个字节（因为只需要写入3字节）
    ptr++;
    // 将3字节写入缓冲区
    memcpy(buff, ptr, 3);

    // 返回写入的字节数
    return 3;
}

// 将2字节的无符号整数转换为网络字节序并写入缓冲区
int BytesWriter::WriteUint16T(char *buff, uint16_t val)
{
    // 将主机字节序转换为网络字节序
    val = htons(val);
    // 将转换后的值写入缓冲区
    memcpy(buff, &val, sizeof(int16_t));

    // 返回写入的字节数
    return sizeof(int16_t);
}

// 将1字节的无符号整数写入缓冲区
int BytesWriter::WriteUint8T(char *buff, uint8_t val)
{
    // 获取值的指针
    char* data = (char*)&val;
    // 将值写入缓冲区的第一个字节
    buff[0] = data[0];

    // 返回写入的字节数
    return 1;
}