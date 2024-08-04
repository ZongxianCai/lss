#include <iostream>
#include <string>
#include "network/base/InetAddress.h"

using namespace lss::network;

int main(int argc, const char **argv)
{
    std::string host;

    while (std::cin >> host)
    {
        InetAddress addr(host);
        std::cout << "host : " << host << std::endl
                << "ip : " << addr.IP() << std::endl
                << "port : " << addr.Port() << std::endl
                << "Lan : " << addr.IsLanIp() << std::endl
                << "Wan : " << addr.IsWanIp() << std::endl
                << "Loop : " << addr.IsLoopbackIp() << std::endl;
    }

    return 0;
}