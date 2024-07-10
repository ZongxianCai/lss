#pragma once
#include <iostream>
#include <string>
#include "NonCopyable.h"

namespace lss
{
    namespace base
    {
        enum LogLevel
        {
            // 常量类型使用k开头
            kTrace,
            kDebug,
            kInfo,
            kWarning,
            kError,
            // 最大日志数量
            kMaxNumOfLogLevel
        };

        // Logger 类的对象不可复制
        class Logger : public NonCopyable
        {
        public:
            // 默认构造函数
            Logger() = default;

            // 默认析构函数
            ~Logger() = default;

            // 设置当前日志级别
            void SetLogLevel(const LogLevel &level);

            // 获取当前日志级别， const 函数用于获取当前的日志级别，该函数是一个常量成员函数，不修改任何成员变量
            LogLevel GetLogLevel() const;

            // 写入日志消息
            void Write(const std::string &msg);

        private:
            // 存储当前日志级别，默认值为 kDebug
            LogLevel level_{kDebug};
        };
    }
}