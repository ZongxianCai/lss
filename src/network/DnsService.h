#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <memory>
#include "network/base/InetAddress.h"
#include "base/NonCopyable.h"
#include "base/Singleton.h"

namespace lss
{
    namespace network
    {
        // 定义InetAddressPtr为InetAddress的智能指针
        using InetAddressPtr = std::shared_ptr<InetAddress>;

        // DnsService类，继承自NonCopyable
        class DnsService : public base::NonCopyable 
        {
        public:
            // 默认构造函数
            DnsService() = default;

            // 添加主机，接受一个常量引用类型的字符串参数 host
            void AddHost(const std::string& host);

            // 从与给定主机名关联的地址列表中检索 IP 地址
            InetAddressPtr GetHostAddress(const std::string &host, int index);

            // 根据给定的主机名返回与之关联的 IP 地址列表
            std::vector<InetAddressPtr> GetHostAddress(const std::string &host);

            // 更新与给定主机名关联的 IP 地址列表，包括间隔、休眠时间和重试次数
            void UpdateHost(const std::string &host, std::vector<InetAddressPtr> &list);

            // 获取当前所有主机及其关联的 IP 地址列表
            std::unordered_map<std::string, std::vector<InetAddressPtr>> GetHosts();

            // 设置DNS服务参数
            void SetDnsServiceParam(int32_t interval, int32_t sleep, int32_t retry);

            // 启动DNS服务
            void Start();

            // 停止DNS服务
            void Stop();

            // 在服务运行时持续获取主机信息并更新
            void OnWork();

            // 静态函数获取主机信息
            static void GetHostInfo(const std::string &host, std::vector<InetAddressPtr> &list);
            
            // 析构函数
            ~DnsService();

        private:
            // 线程对象
            std::thread thread_;

            // 运行状态标志
            bool running_{false};

            // 互斥量，用于线程安全
            std::mutex lock_;

            // 存储主机信息的无序映射
            std::unordered_map<std::string, std::vector<InetAddressPtr>> hosts_info_;

            // 重试次数，默认3次
            int32_t retry_{3};

            // 睡眠时间，单位ms
            int32_t sleep_{200};

            // 间隔时间，默认为6min
            int32_t interval_{180 * 1000};
        };

        // 定义单例DnsService实例
        #define sDnsService lss::base::Singleton<lss::network::DnsService>::Instance()
    }
}