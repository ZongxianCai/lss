#include <iostream>
#include "network/base/InetAddress.h"
#include "network/base/SocketOpt.h"

using namespace lss::network;

void TestClient()
{
    // 创建一个非阻塞的TCP套接字
    int sock = SocketOpt::CreateNonblockingTcpSocket(AF_INET);

    // 检查套接字是否创建成功
    if (sock < 0)
    {
        // 输出错误信息
        std::cerr << "Socket failed. sock : " << sock << " errno : " << errno << std::endl;

        // 退出函数
        return;
    }

    // InetAddress server("192.168.56.101:34444");
    // 定义服务器地址
    InetAddress server("172.22.88.236:34444");
    // 创建SocketOpt对象
    SocketOpt opt(sock);
    // 设置套接字为阻塞模式
    opt.SetNonBlocking(false);
    // 尝试连接到服务器
    auto ret = opt.Connect(server);

    std::cout << "Connect ret : " << ret << std::endl                       // 输出连接结果
            << "errno : " << errno << std::endl                             // 输出错误号
            << "local : " << opt.GetLocalAddr()->ToIpPort() << std::endl    // 输出本地地址
            << "peer : " << opt.GetPeerAddr()->ToIpPort() << std::endl;     // 输出对端地址
}

void TestServer()
{
    // 创建一个非阻塞的TCP套接字
    int sock = SocketOpt::CreateNonblockingTcpSocket(AF_INET);

    // 检查套接字是否创建成功
    if (sock < 0)
    {
        // 输出错误信息
        std::cerr << "Socket failed. sock : " << sock << " errno : " << errno << std::endl;
        
        // 退出函数
        return;
    }

    // 创建一个 InetAddress 对象，表示服务器将监听的地址和端口
    // 0.0.0.0 表示接受来自所有网络接口的连接
    InetAddress server("0.0.0.0:34444");
    // 创建一个 SocketOpt 对象
    SocketOpt opt(sock);
    // 这行代码将套接字设置为阻塞模式，意味着 Accept 调用将会阻塞，直到有客户端连接
    opt.SetNonBlocking(false);
    // 将套接字绑定到之前创建的 server 地址，这样服务器就可以在指定的端口上接收连接
    opt.BindAddress(server);
    // 服务器开始监听传入的连接请求
    opt.Listen();

    // 调用 Accept 方法，等待并接受一个连接
    // 如果有客户端连接，addr 将被填充为连接的客户端地址，ns 将是新的套接字描述符
    InetAddress addr;
    auto ns = opt.Accept(&addr);

    std::cout << "Accept ret : " << ns << std::endl
            << "errno : " << errno << std::endl
            << "addr : " << addr.ToIpPort() << std::endl;  // 输出客户端地址及端口号
}

int main(int argc, const char **argv)
{
    
    // TestClient();
    TestServer();
    return 0;
}