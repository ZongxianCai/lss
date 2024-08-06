#include <iostream>
#include "network/base/InetAddress.h"
#include "network/base/SocketOpt.h"

using namespace lss::network;

void TestClient()
{
    int sock = SocketOpt::CreateNonblockingTcpSocket(AF_INET);

    if (sock < 0)
    {
        std::cerr << "Socket failed. sock : " << sock << " errno : " << errno << std::endl;
        return;
    }

    InetAddress server("192.168.56.101:34444");
    SocketOpt opt(sock);
    opt.SetNonBlocking(false);
    auto ret = opt.Connect(server);

    std::cout << "Connect ret : " << ret << std::endl
            << "errno : " << errno << std::endl
            << "local : " << opt.GetLocalAddr()->ToIpPort() << std::endl
            << "peer : " << opt.GetPeerAddr()->ToIpPort() << std::endl;
}

void TestServer()
{
    int sock = SocketOpt::CreateNonblockingTcpSocket(AF_INET);

    if (sock < 0)
    {
        std::cerr << "Socket failed. sock : " << sock << " errno : " << errno << std::endl;
        return;
    }

    InetAddress server("0.0.0.0:34444");
    SocketOpt opt(sock);
    opt.SetNonBlocking(false);
    auto ret = opt.Connect(server);
    opt.Listen();
    InetAddress addr;
    auto ns = opt.Accept(&addr);

    std::cout << "Accept ret : " << ns << std::endl
            << "errno : " << errno << std::endl
            << "addr : " << addr.ToIpPort() << std::endl;
}

int main(int argc, const char **argv)
{
    
    TestServer();
    return 0;
}