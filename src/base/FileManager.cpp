#include "FileManager.h"

using namespace lss::base;

namespace
{
    static lss::base::FileLogPtr file_log_nullptr;
}

// 实现检查日志文件是否需要切分的回调函数
void FileManager::OnCheck()
{
    bool day_change{false};
    bool hour_change{false};
    // 仅用作测试，实际项目需求中不需要
    bool minute_change{false};

    int year = 0, month = 0, day = -1, hour = -1, minute = 0, second = 0;

    // 获取当前的年 月 日 时 分 秒
    TTime::Now(year, month, day, hour, minute, second);

    // 初始化检查，如果为初始化状态，设置为当前的 日 时
    if (last_day_ == -1)
    {
        last_year_ = year;
        last_month_ = month;
        last_day_ = day;
        last_hour_ = hour;
        // 仅用作测试切分功能
        last_minute_ = minute;
    }

    // 日 变化检查，与上次检查不同，设置标志位为 true
    if (last_day_ != day)
    {
        day_change = true;
    }

    // 时 变化检查，与上次检查不同，设置标志位为 true
    if (last_hour_ != hour)
    {
        hour_change = true;
    }

    if (last_minute_ != minute)
    {
        minute_change = true;
    }

    // 如果 日 时 都没有发生变化，返回，避免执行不必要的操作
    if (!day_change && !hour_change && !minute_change)
    {
        return;
    }

    // 设置互斥锁，确保在遍历和操作 logs_ 时的线程安全
    std::lock_guard<std::mutex> lk(lock_);

    // 遍历 logs_ 中的每个 FileLog 对象，根据 RotateType 和标志位变化决定执行哪种（日/时）切分操作
    for (auto &log : logs_)
    {
        // 仅用作测试
        if (minute_change && log.second->GetRotateType() == kRotateMinute)
        {
            RotateMinutes(log.second);
        }

        if (hour_change && log.second->GetRotateType() == kRotateHour)
        {
            RotateHours(log.second);
        }

        if (day_change && log.second->GetRotateType() == kRotateDay)
        {
            RotateDays(log.second);
        }
    }

    last_year_ = year;
    last_month_ = month;
    last_day_ = day;
    last_hour_ = hour;
}

// 实现根据文件名获取 FileLog 对象的智能指针
FileLogPtr FileManager::GetFileLog(const std::string &fileName)
{
    std::lock_guard<std::mutex> lk(lock_);

    auto iter = logs_.find(fileName);

    if (iter != logs_.end())
    {
        return iter->second;
    }

    FileLogPtr log = std::make_shared<FileLog>();

    if (!log->Open(fileName))
    {
        return file_log_nullptr;
    }

    logs_.emplace(fileName, log);

    return log;
}

// 实现移除指定的 FileLog 对象
void FileManager::RemoveFileLog(const FileLogPtr &log)
{
    std::lock_guard<std::mutex> lk(lock_);
    logs_.erase(log->FilePath());
}

// 实现按照天数执行日志文件的切分
void FileManager::RotateDays(const FileLogPtr &file)
{
    if (file->FileSize() > 0)
    {
        char buff[128] = {0};

        sprintf(buff, "_%04d-%02d-%02d", last_year_, last_month_, last_day_);

        std::string file_path = file->FilePath();
        std::string path = StringUtils::FilePath(file_path);
        std::string file_name = StringUtils::FileName(file_path);
        std::string file_ext = StringUtils::Extension(file_path);

        std::ostringstream oss;

        oss << path
            << file_name
            << buff
            << file_ext;
        
        file->Rotate(oss.str());
    }
}

// 实现按照小时执行日志文件的切分
void FileManager::RotateHours(const FileLogPtr &file)
{
    if (file->FileSize() > 0)
    {
        char buff[128] = {0};

        sprintf(buff, "_%04d-%02d-%02dT%02d", last_year_, last_month_, last_day_, last_hour_);

        std::string file_path = file->FilePath();
        std::string path = StringUtils::FilePath(file_path);
        std::string file_name = StringUtils::FileName(file_path);
        std::string file_ext = StringUtils::Extension(file_path);

        std::ostringstream oss;

        oss << path
            << file_name
            << buff
            << file_ext;

        file->Rotate(oss.str());
    }
}

// 实现按分钟执行日志文件的切分（仅用作测试）
void FileManager::RotateMinutes(const FileLogPtr &file)
{
    if (file->FileSize() > 0)
    {
        char buff[128] = {0};

        sprintf(buff, "_%04d-%02d-%02dT%02d:%02d", last_year_, last_month_, last_day_, last_hour_, last_minute_);

        std::string file_path = file->FilePath();
        std::string path = StringUtils::FilePath(file_path);
        std::string file_name = StringUtils::FileName(file_path);
        std::string file_ext = StringUtils::Extension(file_path);

        std::ostringstream oss;

        oss << path
            << file_name
            << buff
            << file_ext;

        file->Rotate(oss.str());
    }
}
