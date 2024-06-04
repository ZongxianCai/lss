#pragma once

#include <cstdint>
#include <string>
#include <sys/time.h>

namespace lss
{
    namespace base
    {
        class TTime
        {
        public:
            // 表示当前UTC时间，单位：ms
            static int64_t NowMS();

            // 表示当前UTC时间，单位：s
            static int64_t Now();

            // 表示当前时间，返回当前的年月日时分秒
            static int64_t Now(int& year, int& month, int& day, int& hour, int& minute, int& second);
            
            // 返回当前时间ISO格式的字符串（yyyy-MM-ddTHH:mm:ss）
            static std::string ISOTime();
        };
    }
}