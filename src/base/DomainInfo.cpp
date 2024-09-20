#include <fstream>
#include "DomainInfo.h"
#include "base/LogStream.h"
#include "json/json.h"
#include "AppInfo.h"

using namespace lss::base;

// 返回域的名称，返回类型为 const 引用，防止调用者修改名称
const string &DomainInfo::DomainName() const
{
    return name_;
}

// 返回域的类型，返回类型为 const 引用，防止调用者修改类型
const string &DomainInfo::Type() const
{
    return type_;
}

bool DomainInfo::ParseDomainInfo(const std::string &file)
{
    // 输出调试日志，显示要解析的域文件路径
    LOG_DEBUG << " domain file : " << file;

    // 创建一个 Json::Value 对象 root，用于存储解析后的 JSON 数据
    Json::Value root;

    // 创建一个 JSON 读取器构建器，来配置 JSON 解析器的行为
    Json::CharReaderBuilder reader;

    // 打开要解析的文件
    std::ifstream in(file);

    // 用于存储解析过程中的错误信息
    std::string err;

    // 从文件流中解析 JSON 数据，将解析结果存储到 root 中，错误信息存储在 err 中
    auto ok = Json::parseFromStream(reader, in, &root, &err);

    // 如果解析失败，记录错误日志并返回 false
    if (!ok)
    {
        LOG_ERROR << " domain config file : " << file << " parse error. err : " << err;
        return false;
    }

    // 如果 JSON 数据中没有 "domain" 字段，记录错误日志并返回 false
    Json::Value domainObj = root["domain"];
    if (domainObj.isNull())
    {
        LOG_ERROR << " domain config invalid cotent. no domain.";
        return false;
    }  

    // 如果 "name" 字段存在，则将其值赋给 name_
    Json::Value nameObj = domainObj["name"];
    if (!nameObj.isNull())
    {
        name_ = nameObj.asString();
    }

    // 如果 "type" 字段存在，则将其值赋给 type_
    Json::Value typeObj = domainObj["type"];
    if (!typeObj.isNull())
    {
        type_ = typeObj.asString();
    }

    // 如果 "app" 字段不存在，记录错误日志并返回 false
    Json::Value appsObj = domainObj["app"];
    if (appsObj.isNull())
    {
        LOG_ERROR << " domain config invalid cotent.no apps.";
        return false;
    }

    for (auto &aObj : appsObj)
    {
        // 创建一个新的 AppInfo 对象，并使用智能指针 AppInfoPtr 管理该对象的生命周期
        // 构造 AppInfo 对象时，传入当前的 DomainInfo 对象 (*this)
        AppInfoPtr appinfo = std::make_shared<AppInfo>(*this);

        // 调用 AppInfo 的 ParseAppInfo 函数解析应用信息
        auto ret = appinfo->ParseAppInfo(aObj);

        // 如果解析成功，使用互斥锁锁定 appinfos_，并将解析好的 appinfo 插入到 appinfos_ 映射中
        if (ret)
        {
            std::lock_guard<std::mutex> lk(lock_);
            // 键是 appinfo 的应用名称，值是 appinfo 智能指针
            appinfos_.emplace(appinfo->app_name, appinfo);
        }
    }
    
    // 解析成功，返回 true
    return true;
}

AppInfoPtr DomainInfo::GetAppInfo(const string &app_name)
{
    // 使用互斥锁锁定 appinfos_，确保线程安全
    std::lock_guard<std::mutex> lk(lock_);

    // 在 appinfos_ 中查找指定的应用名称
    auto iter = appinfos_.find(app_name);

    // 如果找到，返回对应的 AppInfoPtr 智能指针
    if (iter != appinfos_.end())
    {
        return iter->second;
    }

    // 如果没有找到，返回一个空的智能指针
    return AppInfoPtr();
}