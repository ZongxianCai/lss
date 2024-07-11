#include "Logger.h"

using namespace lss::base;

// 构造函数，接收一个 FileLogPtr 类型的参数，将其用于初始化成员变量 log_
Logger::Logger(const FileLogPtr &log)
:log_(log)
{

}

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
    if (log_)
    {
        log_->WriteLog(msg);
    }
    else
    {
        std::cout << msg;
    }
}