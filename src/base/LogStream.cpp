#include "LogStream.h"

using namespace lss::base;

// 定义一个全局的指针 g_logger ，初始化为 nullptr
Logger *lss::base::g_logger = nullptr;

// 定义一个线程局部变量 thread_id ，用于存储当前线程的唯一标识符，使用 thread_local 关键字确保每个线程有自己的 thread_id 副本
static thread_local pid_t thread_id = 0;

// 日志级别字符串数组，包含不同日志级别的字符串表示
const char *log_string[] = {
    " TRACE ",
    " DEBUG ",
    " INFO ",
    " WARNING ",
    " ERROR "
};

// 接收一个 Logger 指针和文件名、行号、日志级别和函数名，使用这些信息初始化内部的字符串流 stream_。
LogStream::LogStream(Logger *logger, const char *file, int line, LogLevel l, const char *func)
:logger_(logger)
{
    // 从文件名中提取文件名部分，忽略路径
    const char *file_name = strrchr(file, '/');

    if (file_name)
    {
        file_name = file_name + 1;
    }
    else
    {
        file_name = file;
    }

    // 获取当前时间的 ISO 格式字符串
    stream_ << TTime::ISOTime() << " ";
    
    // 为当前线程获取一个唯一的线程 ID，如果尚未获取则使用系统调用 SYS_gettid
    if (0 == thread_id)
    {
        thread_id = static_cast<pid_t>(::syscall(SYS_gettid));
    }
    stream_ << thread_id;

    // 将时间、线程 ID、日志级别字符串、文件名和行号以及可选的函数名格式化到日志消息中
    stream_ << log_string[l];

    stream_ << "[" << file_name << ":" << line << "]";

    if (func)
    {
        stream_ << "[" << func << "]";
    }
}

// 将构建的日志信息发送到 Logger 对象
LogStream::~LogStream()
{
    stream_ << "\n";
    logger_->Write(stream_.str());
}