#pragma once
#include "mmedia/base/MMediaHandler.h"

namespace lss
{
    namespace mm
    {
        class RtmpHandler : public MMediaHandler
        {
        public:
            virtual bool OnPlay(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) 
            {
                return false;
            }

            virtual bool OnPublish(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param) 
            {
                return false;
            }

            virtual void OnPause(const TcpConnectionPtr &conn, bool pause)
            {

            }

            virtual void OnSeek(const TcpConnectionPtr &conn, double time)
            {

            }
        };
    }
}