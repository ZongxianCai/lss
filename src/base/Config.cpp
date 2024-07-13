#include "Config.h"

using namespace lss::base;

// 实现加载和解析配置文件
bool Config::LoadConfig(const std::string &file)
{
    LOG_DEBUG << "Config file: " << file;
    
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::ifstream in(file);
    std::string err;
    auto ok = Json::parseFromStream(reader, in, &root, &err);
    if (!ok)
    {
        LOG_ERROR << "Config file: " << file << " parse error, error: " << err;
        
        return false;
    }

    Json::Value nameObj = root["name"];
    if (!nameObj.isNull())
    {
        name_ = nameObj.asString();
    }

    Json::Value cpusObj = root["cpu_start"];
    if (!cpusObj.isNull())
    {
        cpu_start_ = cpusObj.asInt();
    }

    Json::Value threadsObj = root["threads"];
    if (!threadsObj.isNull())
    {
        thread_nums_ = threadsObj.asInt();
    }

    Json::Value logObj = root["log"];
    if (!logObj.isNull())
    {
        ParseLogInfo(logObj);
    }

    return true;
}

// 实现解析 Json 格式的日志信息，根据 config.json 中的内容进行解析
bool Config::ParseLogInfo(const Json::Value &root)
{
    log_info_ = std::make_shared<LogInfo>();

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
        log_info_->path = pathObj.asString();
    }

    Json::Value nameObj = root["name"];
    if (!nameObj.isNull())
    {
        log_info_->name = nameObj.asString();
    }

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
    return log_info_;
}

// 加载配置，调用 Config 类中的方法实现
bool ConfigManager::LoadConfig(const std::string &file)
{
    LOG_DEBUG << "Load config file: " << file;

    ConfigPtr config = std::make_shared<Config>();
    if (config->LoadConfig(file))
    {
        std::lock_guard<std::mutex> lk(lock_);
        config_ = config;

        return true;
    }

    return false;
}

// 获取配置
ConfigPtr ConfigManager::GetConfig()
{
    std::lock_guard<std::mutex> lk(lock_);

    return config_;
}
