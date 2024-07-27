#include <iostream>
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"

using namespace lss::network;

EventLoopThread event_loop_thread;

void TestEventLoopThread()
{
    event_loop_thread.Run();

    EventLoop *loop = event_loop_thread.Loop();

    if (loop)
    {
        std::cout << "loop: " << loop << std::endl;
    }
}

int main(int argc, const char **argv)
{
    TestEventLoopThread();

    return 0;
}