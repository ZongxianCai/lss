#include "EventLoopThread.h"

using namespace lss::network;

EventLoopThread::EventLoopThread()
: thread_([this](){StartEventLoop();})
{

}

EventLoopThread::~EventLoopThread()
{
    Run();

    if (loop_)
    {
        loop_->Quit();
    }

    if (thread_.joinable())
    {
        thread_.join();
    }
}

void EventLoopThread::Run()
{
    std::call_once(once_, [this](){
        {
            std::lock_guard<std::mutex> lk(lock_);
            running_ = true;
            condition_.notify_one();
        }
        auto f = promise_loop_.get_future();
        f.get();
    });
}

EventLoop *EventLoopThread::loop() const
{
    return loop_;
}

void EventLoopThread::StartEventLoop()
{
    EventLoop loop;
    std::unique_lock<std::mutex> lk(lock_);
    condition_.wait(lk, [this](){return running_;});
    loop_ = &loop;
    promise_loop_.set_value(1);
    loop.Loop();
    loop_ = nullptr;
}
