#pragma once
#include <stdint.h>

namespace lss
{
    namespace mm
    {
        class BytesWriter
        {
        public:
            // 默认构造函数
            BytesWriter() = default;
            
            // 静态方法，将4字节的无符号整数写入缓冲区
            static int WriteUint32T(char *buff, uint32_t val);

            // 静态方法，将3字节的无符号整数写入缓冲区
            static int WriteUint24T(char *buff, uint32_t val);

            // 静态方法，将2字节的无符号整数写入缓冲区
            static int WriteUint16T(char *buff, uint16_t val);

            // 静态方法，将1字节的无符号整数写入缓冲区
            static int WriteUint8T(char *buff, uint8_t val);

            // 默认析构函数
            ~BytesWriter() = default;
        };  
    }  
}
