#include "Logger.h"

using namespace lss::base;

// 设置当前日志级别的实现
void Logger::SetLogLevel(const LogLevel &level)
{
    level_ = level;
}

// 获取当前日志级别的实现
LogLevel Logger::GetLogLevel() const
{
    return level_;
}

// 写入日志消息的实现
void Logger::Write(const std::string &msg)
{
    std::cout << msg;
}