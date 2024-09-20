#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

namespace lss
{
    namespace base
    {
        using std::string;

        // 前向声明 AppInfo 类
        class AppInfo;

        // 定义 AppInfo 类的智能指针类型别名 AppInfoPtr，使用 std::shared_ptr 来管理 AppInfo 对象的生命周期
        using AppInfoPtr = std::shared_ptr<AppInfo>;

        class DomainInfo
        {
        public:
            // 默认构造函数，不做任何初始化工作
            DomainInfo() = default;
            
            // 成员函数，返回域名，返回类型为 const string&，表示返回常量引用，防止调用者修改值
            const string &DomainName() const;

            // 成员函数，返回域类型，返回类型为 const string&
            const string &Type() const;

            // 成员函数，用于解析指定文件中的域信息，参数是文件名，返回值为布尔类型，表示解析是否成功
            bool ParseDomainInfo(const std::string &file);

            // 成员函数，通过应用名称获取对应的 AppInfo 智能指针对象，如果存在则返回对应的 AppInfoPtr
            AppInfoPtr GetAppInfo(const string &app_name);

            // 默认析构函数，负责释放资源，当不需要额外处理资源时，使用默认析构函数
            ~DomainInfo() = default;

        private:
            // 成员变量，存储域的名称
            string name_;

            // 成员变量，存储域的类型
            string type_;

            // 成员变量，互斥锁，用于保证多线程环境下对 appinfos_ 的安全访问
            std::mutex lock_;
            
            // 成员变量，使用无序映射存储应用信息，键是应用名称，值是指向 AppInfo 的智能指针
            std::unordered_map<string, AppInfoPtr> appinfos_;
        };
    }
}