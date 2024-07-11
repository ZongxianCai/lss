#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace lss
{
    namespace base
    {
        // 日志切分类型
        enum RotateType
        {
            // 不切分
            kRotateNone,

            // 按分钟切分日志文件（仅用作测试切分功能是否正常）
            kRotateMinute,

            // 按小时切分日志文件
            kRotateHour,

            // 按天数切分日志文件
            kRotateDay
        };

        class FileLog
        {
        public:
            // 默认构造函数
            FileLog() = default;

            // 默认析构函数
            ~FileLog() = default;

            // 打开日志文件，参数为文件路径
            bool Open(const std::string &filePath);

            // 写入日志消息，参数为要写入的消息字符串
            size_t WriteLog(const std::string &msg);

            // 执行日志文件的切分操作，参数为文件名
            void Rotate(const std::string &file);

            // 设置日志文件的文件类型
            void SetRotate(const RotateType type);

            // 获取当前设置的日志文件切分类型，该操作只是读取值，不会对 FileLog 对象的状态产生任何影响，将其声明为const，可以在 const 限定的对象上被调用
            RotateType GetRotateType() const;

            // 获取日志文件的大小，该操作只是读取值，不会对 FileLog 对象的状态产生任何影响，将其声明为const，可以在 const 限定的对象上被调用
            int64_t FileSize() const;
            
            // 获取文件路径
            std::string FilePath() const;

        private:
            // 存储文件描述符
            int fd_{-1};

            // 存储日志文件路径
            std::string file_path_;

            // 切分类型
            RotateType rotate_type_{kRotateNone};
        };

        // 定义了一个类型别名，使用 std::shared_ptr 智能管理 FileLog 对象的生命周期
        using FileLogPtr = std::shared_ptr<FileLog>;
    }
}