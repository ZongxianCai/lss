#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <sstream>
#include "Singleton.h"
#include "FileLog.h"
#include "TTime.h"
#include "StringUtils.h"

namespace lss 
{
    namespace base 
    {
        // 继承 NonCopyable 类，用于禁止复制构造函数和赋值操作，确保 FileManager 的实例唯一
        class FileManager : public NonCopyable
        {
        public:
            // 默认构造函数
            FileManager() = default;

            // 默认析构函数
            ~FileManager() = default;

            // 检查日志文件是否需要切分的回调函数
            void OnCheck();

            // 根据文件名获取 FileLog 对象的智能指针
            FileLogPtr GetFileLog(const std::string &fileName);

            // 移除指定的 FileLog 对象
            void RemoveFileLog(const FileLogPtr &log);

            // 按照天数执行日志文件的切分
            void RotateDays(const FileLogPtr &file);

            // 按照小时执行日志文件的切分
            void RotateHours(const FileLogPtr &file);

            // 按照小时执行日志文件的切分（仅用作测试）
            void RotateMinutes(const FileLogPtr &file);

        private:
            // 存储文件名到 FilePtr 的映射
            std::unordered_map<std::string, FileLogPtr> logs_;

            // 同步访问共享资源，确保线程安全
            std::mutex lock_;

            // 记录上一次执行按年切分的时间，初始化为 -1 表示尚未进行切分
            int last_year_{-1};

            // 记录上一次执行按月切分的时间，初始化为 -1 表示尚未进行切分
            int last_month_{-1};

            // 记录上一次执行按天切分的时间，初始化为 -1 表示尚未进行切分
            int last_day_{-1};

            // 记录上一次执行按小时切分的时间，初始化为 -1 表示尚未进行切分
            int last_hour_{-1};

            // 记录上一次执行按分钟切分的时间，初始化为 -1 表示尚未进行切分（仅用作测试）
            int last_minute_{-1};
        };
    }
}

// 使用宏定义提供一个全局访问点 fileManager ，以便访问 FileManager 类的单例实例
#define fileManager lss::base::Singleton<lss::base::FileManager>::Instance()