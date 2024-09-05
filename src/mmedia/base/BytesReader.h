#pragma once
#include <stdint.h>

namespace lss
{
    namespace mm
    {
        class BytesReader
        {
        public:
            // 默认构造函数
            BytesReader() = default;

            // 静态方法，读取4字节的无符号整数
            static uint32_t ReadUint32T(const char *data);

            // 静态方法，读取3字节的无符号整数
            static uint32_t ReadUint24T(const char *data);

            // 静态方法，读取2字节的无符号整数
            static uint16_t ReadUint16T(const char *data);

            // 静态方法，读取1字节的无符号整数
            static uint8_t  ReadUint8T(const char *data);

            // 默认析构函数
            ~BytesReader() = default;
        }; 
    }   
}
