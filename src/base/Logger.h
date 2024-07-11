#pragma once
#include <iostream>
#include <string>
#include "NonCopyable.h"
#include "FileLog.h"

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
            // 构造函数，接收一个 FileLogPtr 类型的参数，将其用于初始化成员变量 log_
            Logger(const FileLogPtr &log);

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

            // 声明一个名为 log_ 的成员变量，类型为 FileLogPtr 
            // log_ 可以持有一个 FileLog 对象的引用，并自动管理该对象的生命周期
            FileLogPtr log_;
        };
    }
}