#pragma once
#include <string>
#include <memory>
#include "json/json.h"

namespace lss
{
    namespace base
    {
        using std::string;

        // 前向声明 DomainInfo 类
        class DomainInfo;

        class AppInfo
        {
        public:
            // 构造函数，使用 explicit 关键字来防止隐式转换，接收一个 DomainInfo 类型的引用 d，并初始化类中的 domain_info 成员
            explicit AppInfo(DomainInfo &d);

            // ParseAppInfo 函数，用于解析 JSON 数据，接收一个 Json::Value 类型的对象 root（表示解析的 JSON 数据的根节点）
            bool ParseAppInfo(Json::Value &root);

            // 引用类型的成员变量 domain_info，表示该 AppInfo 对象关联的 DomainInfo 对象
            DomainInfo &domain_info;

            // 字符串类型成员变量，表示域名信息
            std::string domain_name;

            // 字符串类型成员变量，表示应用名称
            std::string app_name;

            // 无符号 32 位整型成员变量，表示最大缓冲区大小，初始化为 1000
            uint32_t max_buffer{1000};

            // 布尔类型成员变量，表示是否支持 RTMP 协议，默认值为 false
            bool rtmp_support{false};

            // 布尔类型成员变量，表示是否支持 FLV 格式，默认值为 false
            bool flv_support{false};

            // 布尔类型成员变量，表示是否支持 HLS 协议，默认值为 false
            bool hls_support{false};

            // 无符号 32 位整型成员变量，表示内容的延迟时间，单位为毫秒，默认值为 3000 毫秒（3 秒）
            uint32_t content_latency{3*1000};

            // 无符号 32 位整型成员变量，表示流的空闲超时时间，单位为毫秒，默认值为 30000 毫秒（30 秒）
            uint32_t stream_idle_time{30*1000};
            
            // 无符号 32 位整型成员变量，表示流的超时时间，单位为毫秒，默认值为 30000 毫秒（30 秒）
            uint32_t stream_timeout_time{30*1000};
        };
    }
}