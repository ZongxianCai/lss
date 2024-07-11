#include <thread>
#include "Logger.h"
#include "LogStream.h"
#include "FileLog.h"
#include "FileManager.h"
#include "TTime.h"
#include "TaskManager.h"

using namespace lss::base;

std::thread t;

void TestLog()
{
    t = std::thread([](){
        while (true)
        {
            LOG_INFO << " Test Info !!! Now: " << TTime::NowMS();
            LOG_DEBUG << " Test Debug !!! Now: " << TTime::NowMS();

            // 使用线程，每隔500ms输出一次
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
}

/*
void TestLog()
{
    LOG_TRACE << " Test Trace !!! ";
    LOG_DEBUG << " Test Debug !!! ";
    LOG_INFO << " Test Info !!! ";
    LOG_WARNING << " Test Warning !!! ";
    LOG_ERROR << " Test Error !!! ";
}
*/

int main(int argc, const char **argv)
{
    FileLogPtr log = fileManager->GetFileLog("test.log");
    
    // 设置切分条件
    log->SetRotate(kRotateMinute);
    
    lss::base::g_logger = new Logger(log);

    lss::base::g_logger->SetLogLevel(kTrace);

    TaskPtr task = std::make_shared<Task>([](const TaskPtr &task)
                                            {
                                                fileManager->OnCheck();
                                                task->Restart();
                                            },
                                            1000);

    taskManager->Add(task);

    TestLog();

    // 启动定时任务

    while (true)
    {
        taskManager->OnWork();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}