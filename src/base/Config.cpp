#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "Config.h"
#include "AppInfo.h"
#include "DomainInfo.h"

using namespace lss::base;

namespace
{
    // 定义一个空的 ServiceInfoPtr 智能指针，便于在没有匹配时返回
    static ServiceInfoPtr service_info_nullptr;
}

// 实现加载和解析配置文件
bool Config::LoadConfig(const std::string &file)
{
    // 输出调试日志，显示正在加载的配置文件
    LOG_DEBUG << " Config file : " << file;
    
    // 用于存储解析的 JSON 根对象
    Json::Value root;

    // JSON 解析器
    Json::CharReaderBuilder reader;

    // 打开文件输入流，用于读取配置文件
    std::ifstream in(file);

    // 用于存储解析错误信息
    std::string err;

    // 从文件流中解析 JSON
    auto ok = Json::parseFromStream(reader, in, &root, &err);

    // 如果解析失败，记录错误日志并返回 false
    if (!ok)
    {
        LOG_ERROR << " Config file : " << file << " parse error, error : " << err;
        return false;
    }

    // 解析配置中的 "name" 字段，如果 "name" 存在，设置 name_ 成员变量
    Json::Value nameObj = root["name"];
    if (!nameObj.isNull())
    {
        name_ = nameObj.asString();
    }

    // 解析 "cpu_start" 字段，如果存在，设置 cpu_start_
    Json::Value cpusObj = root["cpu_start"];
    if (!cpusObj.isNull())
    {
        cpu_start_ = cpusObj.asInt();
    }

    // 解析 "cpus" 字段，如果存在，设置 cpus_ 成员变量
    Json::Value cpus1Obj = root["cpus"];
    if (!cpus1Obj.isNull())
    {
        cpus_ = cpus1Obj.asInt();
    }

    // 解析 "threads" 字段，如果存在，设置线程数量 thread_nums_
    Json::Value threadsObj = root["threads"];
    if (!threadsObj.isNull())
    {
        thread_nums_ = threadsObj.asInt();
    }

    // 解析 "log" 字段，如果日志字段存在，调用 ParseLogInfo 方法解析日志信息
    Json::Value logObj = root["log"];
    if (!logObj.isNull())
    {
        ParseLogInfo(logObj);
    }

    // 解析 "services" 字段，如果服务信息解析失败，返回 false
    if (!ParseServiceInfo(root["services"]))
    {
        return false;
    }

    // 解析目录信息
    ParseDirectory(root["directory"]);

    return true;
}

// 实现解析 Json 格式的日志信息，根据 config.json 中的内容进行解析
bool Config::ParseLogInfo(const Json::Value &root)
{
    // 创建 LogInfo 的智能指针
    log_info_ = std::make_shared<LogInfo>();

    // 解析日志级别并根据不同的字符串值设置日志级别
    Json::Value levelObj = root["level"];
    if (!levelObj.isNull())
    {
        std::string level = levelObj.asString();
        if (level == "TRACE")
        {
            log_info_->level = kTrace;
        }
        else if (level == "DEBUG")
        {
            log_info_->level = kDebug;
        }
        else if (level == "INFO")
        {
            log_info_->level = kInfo;
        }
        else if (level == "WARNING")
        {
            log_info_->level = kWarning;
        }
        else if (level == "ERROR")
        {
            log_info_->level = kError;
        }                              
    }

    Json::Value pathObj = root["path"];
    if (!pathObj.isNull())
    {
        // 设置日志文件路径
        log_info_->path = pathObj.asString();
    }

    Json::Value nameObj = root["name"];
    if (!nameObj.isNull())
    {
        // 设置日志文件名
        log_info_->name = nameObj.asString();
    }

    // 解析日志滚动类型，设置为按天或按小时滚动
    Json::Value rtObj = root["rotate"];
    if (!rtObj.isNull())
    {
        std::string rt = rtObj.asString();
        if (rt == "DAY")
        {
            log_info_->rotate_type = kRotateDay;
        }
        else if (rt == "HOUR")
        {
            log_info_->rotate_type = kRotateHour;
        }                         
    }

    return true;      
}

// 实现获取日志信息
LogInfoPtr &Config::GetLogInfo()
{
    // 返回日志信息的智能指针
    return log_info_;
}

const std::vector<ServiceInfoPtr> &Config::GetServiceInfos()
{
    // 返回服务信息列表
    return services_;
}

const ServiceInfoPtr &Config::GetServiceInfo(const string &protocol, const string &transport)
{
    // 遍历服务信息，根据协议和传输层协议找到匹配的服务信息并返回
    for (auto &s : services_)
    {
        if (s->protocol == protocol && s->transport == transport)
        {
            return s;
        }
    }

    // 如果未找到，返回空指针
    return service_info_nullptr;
}

// 解析服务信息，解析成功则将服务信息添加到 services_ 容器中
bool Config::ParseServiceInfo(const Json::Value &serviceObj)
{
    // 如果配置文件中没有 service 段，记录错误并返回 false
    if (serviceObj.isNull())
    {
        LOG_ERROR << " config no service section!";
        return false;
    }

    // 如果 service 段不是数组类型，记录错误并返回 false
    if (!serviceObj.isArray())
    {
        LOG_ERROR << " service section type is not array!";
        return false;
    }

    // 遍历解析的服务信息
    for (auto const &s : serviceObj)
    {
        // 创建 ServiceInfo 的智能指针
        ServiceInfoPtr sinfo = std::make_shared<ServiceInfo>();

        // 解析每个服务的地址、端口、协议和传输层协议
        sinfo->addr = s.get("addr", "0.0.0.0").asString();
        sinfo->port = s.get("port", "0").asInt();
        sinfo->protocol = s.get("protocol", "rtmp").asString();
        sinfo->transport = s.get("transport", "tcp").asString();

        // 记录解析的服务信息
        LOG_INFO << " service info addr : " << sinfo->addr << " port : " << sinfo->port << " protocol : " << sinfo->protocol << " transport : " << sinfo->transport;

        // 将解析后的服务信息添加到 services_ 容器中
        services_.emplace_back(sinfo);
    }

    // 解析成功
    return true;
}

bool Config::ParseDirectory(const Json::Value &root)
{
    // 如果目录为空或不是数组，返回 false
    if (root.isNull() || !root.isArray())
    {
        return false;
    }
    
    for (const Json::Value &d : root)
    {
        // 获取目录路径
        std::string path = d.asString();

        // 创建 stat 结构体，用于检查路径状态
        struct stat st;

        // 输出日志
        LOG_TRACE << " ParseDirectory path : " << path;

        // 调用 stat 函数检查路径状态
        auto ret = stat(path.c_str(), &st);

        // 输出日志
        LOG_TRACE << " ret : "<< ret << " errno : " << errno;
        
        if (ret != -1)
        {
            // 如果是目录，则调用 ParseDomainPath 解析目录
            if ((st.st_mode &S_IFMT) == S_IFDIR)
            {
                ParseDomainPath(path);
            }
            // 如果是文件，则调用 ParseDomainFile 解析文件
            else if ((st.st_mode &S_IFMT) == S_IFREG)
            {
                ParseDomainFile(path);
            }
        }
    }
    return true;
}

bool Config::ParseDomainPath(const string &path)
{
    // 定义目录流指针
    DIR *dp = nullptr;

    // 定义目录项结构体
    struct dirent * pp = nullptr;

    // 输出日志
    LOG_DEBUG << " parse domain path : " << path;

    // 打开目录
    dp = opendir(path.c_str());

    // 如果目录打开失败，返回 false
    if (dp == nullptr)
    {
        return false;
    }

    while (true)
    {
        // 读取目录项
        pp = readdir(dp);

        // 读取完所有项后退出循环
        if (pp == nullptr)
        {
            break;
        }

        // 跳过 "." 目录
        if (pp->d_name[0] == '.')
        {
            continue;
        }

        // 如果是常规文件，解析该文件
        if (pp->d_type == DT_REG)
        {
            if (path.at(path.size() - 1) != '/')
            {
                ParseDomainFile(path + "/" + pp->d_name);
            }
            else 
            {
                ParseDomainFile(path + pp->d_name);
            }
        }
    }

    // 关闭目录流
    closedir(dp);

    return true;
}

bool Config::ParseDomainFile(const string &file)
{
    // 输出调试日志，显示当前正在解析的域名文件
    LOG_DEBUG << " parse domain file : " << file;

    // 创建一个 DomainInfo 对象的智能指针，用于保存解析后的域名信息
    DomainInfoPtr d = std::make_shared<DomainInfo>();

    // 调用 DomainInfo 对象的 ParseDomainInfo 方法来解析域名文件内容
    auto ret = d->ParseDomainInfo(file);

    if (ret)
    {
        // 加锁以保护 domaininfos_ 共享数据结构
        std::lock_guard<std::mutex> lk(lock_);

        // 在 domaininfos_ 哈希表中查找已解析的域名
        auto iter = domaininfos_.find(d->DomainName());

        // 如果找到同名的域名，先删除旧的域名信息
        if (iter != domaininfos_.end())
        {
            domaininfos_.erase(iter);
        }

        // 将新的域名信息插入到哈希表中，使用域名作为键
        domaininfos_.emplace(d->DomainName(), d);
    }
    // 返回 true，表示解析成功
    return true;
}

AppInfoPtr Config::GetAppInfo(const string &domain, const string &app)
{
    // 加锁以确保线程安全，保护 domaininfos_ 共享数据结构
    std::lock_guard<std::mutex> lk(lock_);

    // 在 domaininfos_ 哈希表中查找指定域名
    auto iter = domaininfos_.find(domain);

    // 如果找到指定域名，调用该域名的 GetAppInfo 方法，返回对应的应用信息
    if (iter != domaininfos_.end())
    {
        return iter->second->GetAppInfo(app);
    }

    // 如果未找到指定域名，返回空的 AppInfoPtr
    return AppInfoPtr();
}

DomainInfoPtr Config::GetDomainInfo(const string &domain)
{
    // 加锁以确保线程安全，保护 domaininfos_ 共享数据结构
    std::lock_guard<std::mutex> lk(lock_);

    // 在 domaininfos_ 哈希表中查找指定域名
    auto iter = domaininfos_.find(domain);

    // 如果找到指定域名，返回该域名的智能指针
    if (iter != domaininfos_.end())
    {
        return iter->second;
    }

    // 如果未找到指定域名，返回空的 DomainInfoPtr
    return DomainInfoPtr();
}

// 加载配置，调用 Config 类中的方法实现
bool ConfigManager::LoadConfig(const std::string &file)
{
    // 输出调试日志，显示当前正在加载的配置文件
    LOG_DEBUG << " Load config file : " << file;

    // 创建一个 Config 对象的智能指针，用于保存配置文件中的配置信息
    ConfigPtr config = std::make_shared<Config>();

    if (config->LoadConfig(file))
    {
        // 加锁以确保线程安全，保护 config_ 共享数据结构
        std::lock_guard<std::mutex> lk(lock_);

        // 将加载成功的配置赋值给成员变量 config_
        config_ = config;

        // 返回 true，表示加载成功
        return true;
    }

    // 如果加载失败，返回 false
    return false;
}

// 获取配置
ConfigPtr ConfigManager::GetConfig()
{
    // 加锁以确保线程安全，保护 config_ 共享数据结构
    std::lock_guard<std::mutex> lk(lock_);
    // 返回当前加载的配置的智能指针
    return config_;
}
