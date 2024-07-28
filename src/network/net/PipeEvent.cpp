#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "PipeEvent.h"
#include "network/base/Network.h"

using namespace lss::network;

// 构造函数，接收一个 指向 EventLoop 对象的指针 loop 作为参数
PipeEvent::PipeEvent(EventLoop *loop)
: Event(loop)           // 调用基类 Event 的构造函数进行初始化
{
    int fd[2] = {0, };
    // 调用pipe2函数创建一个管道，并将文件描述符存储在fd数组中
    // pipe2 函数的第一个参数是一个整型数组，用于存储读取和写入端的文件描述符，第二个参数O_NONBLOCK表示以非阻塞模式打开管道
    auto ret = ::pipe2(fd, O_NONBLOCK);
    // 判断管道是否创建成功，如果失败，则输出错误信息并退出程序
    if (ret < 0)
    {
        NETWORK_ERROR << " pipe open failed. ";
        exit(-1);
    }
    // 将读取端的文件描述符存储在成员变量 fd_ 中
    fd_ = fd[0];
    // 将写入端的文件描述符存储在成员变量 write_fd_ 中
    write_fd_ = fd[1];
}

// 析构函数，确保在对象销毁时，相关的资源得到正确的释放   
PipeEvent::~PipeEvent()
{
    // 写入端的文件描述符大于 0 时，关闭写入端的文件描述符
    if (write_fd_ > 0)
    {
        ::close(write_fd_);
        // 将write_fd_的值设置为-1，表示文件描述符已关闭
        write_fd_ = -1;
    }
}

// 处理管道读取事件
void PipeEvent::OnRead()
{
    int64_t tmp = 0;
    // 调用系统函数 read 从文件描述符 fd_ 中读取数据，并将读取的数据存储到 tmp 变量中
    auto ret = ::read(fd_, &tmp, sizeof(tmp));
    // 如果读取操作返回值小于0，表示读取出错，输出错误信息并返回
    if (ret < 0)
    {
        NETWORK_ERROR << " pipe read error. error : " << errno;

        return;
    }
    // 如果读取成功，则输出读取到的数据
    std::cout << "pipe read success. tmp : " << tmp << std::endl;
}

// 处理管道关闭事件
void PipeEvent::OnClose()
{
    if (write_fd_ > 0)
    {
        ::close(write_fd_);
        write_fd_ = -1;
    }
}

// 处理管道错误事件，将错误信息输出到标准输出流中
void PipeEvent::OnError(const std::string &msg)
{
    std::cout << "error : " << msg << std::endl;
}

// 向管道写入数据，data 为要写入的数据,len 为要写入的数据长度
void PipeEvent::Write(const char *data, size_t len)
{
    // 调用系统函数 write 将 data 中的数据写入到文件描述符 write_fd_ 所代表的管道中
    ::write(write_fd_, data, len);
}