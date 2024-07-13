#pragma once
#include <iostream>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "TTime.h"
#include "Logger.h"

namespace lss
{
    namespace base
    {
        // 在当前文件中引用全局变量 g_logger
        extern Logger *g_logger;

        class LogStream
        {
        public:
            // 接收一个logger指针、文件名、行号、日志级别和函数名作为参数
            LogStream(Logger *logger, const char *file, int line, LogLevel l, const char *func = nullptr);

            // 清理LogStream对象
            ~LogStream();

            // 模板函数，允许 LogStream 对象使用 << 操作符接收不同类型的数据，并将其添加到内部的字符串流 stream_ 中
            template<class T> LogStream &operator<<(const T& value)
            {
                stream_ << value;
                
                return *this;
            }
        
        private:
            // 构建最终的日志消息
            std::ostringstream stream_;
            
            // 指向 Logger 类的指针，初始化为 nullptr，该指针用于引用一个 Logger 对象， LogStream 可以通过该指针调用 Logger 的方法来输出日志
            Logger *logger_{nullptr};
        };
    }
}

#define LOG_TRACE   \
    if (g_logger && lss::base::g_logger->GetLogLevel() <= lss::base::kTrace)    \
        lss::base::LogStream(lss::base::g_logger, __FILE__, __LINE__, lss::base::kTrace, __func__)

#define LOG_DEBUG   \
    if (g_logger && lss::base::g_logger->GetLogLevel() <= lss::base::kDebug)    \
        lss::base::LogStream(lss::base::g_logger, __FILE__, __LINE__, lss::base::kDebug, __func__)

#define LOG_INFO    \
    if (g_logger && lss::base::g_logger->GetLogLevel() <= lss::base::kInfo)     \
        lss::base::LogStream(lss::base::g_logger, __FILE__, __LINE__, lss::base::kInfo)

#define LOG_WARNING     \
    if (g_logger && lss::base::g_logger->GetLogLevel() <= lss::base::kWarning)     \
        lss::base::LogStream(lss::base::g_logger, __FILE__, __LINE__, lss::base::kWarning)

#define LOG_ERROR   \
    lss::base::LogStream(lss::base::g_logger, __FILE__, __LINE__, lss::base::kError)
