#pragma once
#include "mmedia/base/MMediaHandler.h"

namespace lss
{
    namespace mm
    {
        class RtmpHandler : public MMediaHandler
        {
        public:
            // 处理播放请求的虚函数，接收连接对象、会话名称和参数
            virtual bool OnPlay(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) 
            {
                // 默认返回 false，表示未处理
                return false;
            }

            // 处理发布请求的虚函数，接收连接对象、会话名称和参数
            virtual bool OnPublish(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) 
            {
                // 默认返回 false，表示未处理
                return false;
            }

            // 处理暂停请求的虚函数，接收连接对象和暂停标志
            virtual void OnPause(const TcpConnectionPtr &conn, bool pause)
            {

            }

            // 处理寻求（快进或回退）请求的虚函数，接收连接对象和时间点
            virtual void OnSeek(const TcpConnectionPtr &conn, double time)
            {

            }
        };
    }
}