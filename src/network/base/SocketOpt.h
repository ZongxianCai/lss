#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory>
#include "InetAddress.h"

namespace lss
{
    namespace network
    {
        using InetAddressPtr = std::shared_ptr<InetAddress>;

        class SocketOpt
        {
        public:
            SocketOpt(int sock, bool v6 = false);

            static int CreateNonblockingTcpSocket(int family);

            static int CreateNonblockingUdpSocket(int family);
            
            int BindAddress(const InetAddress &localaddr);

            int Listen();

            int Accept(InetAddress *peeraddr);

            int Connect(const InetAddress &addr);

            InetAddressPtr GetLocalAddr();

            InetAddressPtr GetPeerAddr();

            void SetTcpNoDelay(bool on);

            void SetReuseAddr(bool on);

            void SetReusePort(bool on);

            void SetKeepAlive(bool on);

            void SetNonBlocking(bool on);

            ~SocketOpt() = default;

        private:
            int sock_;
            bool is_v6_{false};
        };
    }
}