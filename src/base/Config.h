#pragma once
#include <iostream>
#include <string>
#include <cstdint>
#include <memory>
#include <mutex>
#include <fstream>
#include <unordered_map>
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
        using std::string;

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

        // 服务器信息
        struct ServiceInfo
        {
            // 存储服务的地址，例如 IP 地址或域名
            string addr;

            // 服务的端口号，数据类型为 16 位无符号整数，用于网络端口
            uint16_t port;

            // 服务所使用的协议，例如 "HTTP"、"RTMP" 等
            string protocol;

            // 服务的传输层协议，例如 "TCP" 或 "UDP"
            string transport;
        };

        // 使用智能指针来管理 ServiceInfo 对象，避免手动管理内存泄漏
        using ServiceInfoPtr = std::shared_ptr<ServiceInfo>;


        // 定义了一个类型别名 LogInfoPtr ，是 std::shared_ptr<LogInfo> 的简写
        using LogInfoPtr = std::shared_ptr<LogInfo>;

        // 前向声明
        class DomainInfo;

        // 前向声明
        class AppInfo;

        // 定义 DomainInfo 类的智能指针别名
        using DomainInfoPtr = std::shared_ptr<DomainInfo>;

        // 定义 AppInfo 类的智能指针别名
        using AppInfoPtr = std::shared_ptr<AppInfo>;

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

            // 函数声明，返回所有 ServiceInfo 的智能指针的常量引用，返回类型是 std::vector<ServiceInfoPtr>
            const std::vector<ServiceInfoPtr> & GetServiceInfos();

            // 函数声明，根据协议和传输层协议获取特定的 ServiceInfo，返回类型是 ServiceInfoPtr 的常量引用
            const ServiceInfoPtr &GetServiceInfo(const string &protocol, const string &transport);
            
            // 函数声明，解析服务信息，参数是 JSON 对象 serviceObj，返回值是布尔类型，表示解析是否成功
            bool ParseServiceInfo(const Json::Value &serviceObj);

            // 函数声明，根据域名和应用名获取 AppInfo 的智能指针，返回类型是 AppInfoPtr
            AppInfoPtr GetAppInfo(const string &domain, const string &app);

            // 函数声明，根据域名获取 DomainInfo 的智能指针，返回类型是 DomainInfoPtr
            DomainInfoPtr GetDomainInfo(const string &domain);

            // 存储配置文件的名称
            std::string name_;
            
            // CPU的起始编号
            int32_t cpu_start_{0};
            
            // 线程数
            int32_t thread_nums_{1};

            // CPU数
            int32_t cpus_{1};

        private:
            // 函数声明，解析目录结构，参数是 JSON 对象 root，返回值是布尔类型，表示解析是否成功
            bool ParseDirectory(const Json::Value &root);

            // 函数声明，解析域名的路径，参数是字符串 path，返回值是布尔类型，表示解析是否成功
            bool ParseDomainPath(const string &path);

            // 函数声明，解析域名的配置文件，参数是文件名 file，返回值是布尔类型，表示解析是否成功
            bool ParseDomainFile(const string &file);

            // 解析 Json 格式的日志信息
            bool ParseLogInfo(const Json::Value &root);

            // 指向 LogInfo 的智能指针，用于存储解析后的日志信息
            LogInfoPtr log_info_;

            // 存储多个 ServiceInfoPtr 智能指针对象的列表，用于管理所有服务信息
            std::vector<ServiceInfoPtr> services_;

            // 使用无序映射存储域信息，键是域名，值是 DomainInfoPtr 智能指针，便于快速查找域信息
            std::unordered_map<std::string, DomainInfoPtr> domaininfos_;

            // 互斥锁，用于确保在多线程环境下对 domaininfos_ 和其他共享数据的安全访问
            std::mutex lock_;
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