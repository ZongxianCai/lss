#pragma once
#include <iostream>
#include <string>
#include <cstdint>
#include <memory>
#include <mutex>
#include <fstream>
#include "json/json.h"
#include "LogStream.h"
#include "NonCopyable.h"
#include "Singleton.h"
#include "Logger.h"
#include "FileLog.h"

namespace lss
{
    namespace base
    {
        // 日志信息
        struct LogInfo
        {
            // 打印级别
            LogLevel level;
            
            // 打印文件的目录
            std::string path;
            
            // 文件名
            std::string name;

            // 切分条件，默认不切分
            RotateType rotate_type{kRotateNone};

        };

        // 定义了一个类型别名 LogInfoPtr ，是 std::shared_ptr<LogInfo> 的简写
        using LogInfoPtr = std::shared_ptr<LogInfo>;

        class Config
        {
        public:
            // 使用默认构造函数
            Config() = default;

            // 使用默认析构函数
            ~Config() = default;

            // 加载和解析配置文件
            bool LoadConfig(const std::string &file);

            // 获取日志信息
            LogInfoPtr &GetLogInfo();

            // 存储配置文件的名称
            std::string name_;
            
            // CPU的起始编号
            int32_t cpu_start_{0};
            
            // 线程数
            int32_t thread_nums_{1};

        private:
            // 解析 Json 格式的日志信息
            bool ParseLogInfo(const Json::Value &root);

            // 指向 LogInfo 的智能指针，用于存储解析后的日志信息
            LogInfoPtr log_info_;
        };

        // 定义了一个类型别名 ConfigPtr ，是 std::shared_ptr<Config> 的简写
        using ConfigPtr = std::shared_ptr<Config>;

        class ConfigManager : public NonCopyable
        {
        public:
            // 默认构造函数
            ConfigManager() = default;

            // 默认析构函数
            ~ConfigManager() = default;

            // 加载配置，调用 Config 类中的方法实现
            bool LoadConfig(const std::string &file);

            // 获取配置
            ConfigPtr GetConfig();

        private:
            // 配置指针
            ConfigPtr config_;

            // 锁
            std::mutex lock_;
        };

        // 使用宏定义提供一个全局访问点 configManager ，以便访问 ConfigManager 类的单例实例
        #define configManager lss::base::Singleton<lss::base::ConfigManager>::Instance()
    }
}