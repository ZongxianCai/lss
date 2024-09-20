#include "AppInfo.h"
#include "base/LogStream.h"
#include "DomainInfo.h"

using namespace lss::base;

AppInfo::AppInfo(DomainInfo &d)
    : domain_info(d)
{

}

bool AppInfo::ParseAppInfo(Json::Value &root)
{
    Json::Value nameObj = root["name"];
    if (!nameObj.isNull())
    {
        app_name = nameObj.asString();
    }

    Json::Value mbObj = root["max_buffer"];
    if (!mbObj.isNull())
    {
        max_buffer = mbObj.asUInt();
    }   

    Json::Value hlsObj = root["hls_support"];
    if (!hlsObj.isNull())
    {
        hls_support = hlsObj.asString() == "on";
    }

    Json::Value flvObj = root["flv_support"];
    if (!flvObj.isNull())
    {
        flv_support = flvObj.asString() == "on";
    }

    Json::Value rtmpObj = root["rtmp_support"];
    if (!rtmpObj.isNull())
    {
        rtmp_support = rtmpObj.asString() == "on";
    }  

    Json::Value clObj = root["content_latency"];
    if (!clObj.isNull())
    {
        content_latency = clObj.asUInt()*1000;
    }  

    Json::Value sitObj = root["stream_idle_time"];
    if(!sitObj.isNull())
    {
        stream_idle_time = sitObj.asUInt();
    } 

    Json::Value sttObj = root["stream_timeout_time"];
    if(!sttObj.isNull())
    {
        stream_timeout_time = sttObj.asUInt();
    }      

    LOG_INFO << " app name : " << app_name
            << " max_buffer : " << max_buffer
            << " content_latency : " << content_latency
            << " stream_idle_time : "<< stream_idle_time
            << " stream_timeout_time : " << stream_timeout_time
            << " rtmp_support : " << rtmp_support
            << " flv_support : " << flv_support
            << " hls_support : " << hls_support;
            
    return true;            
}