#include <iostream>
#include <thread>
#include "base/Config.h"
#include "base/LogStream.h"
#include "base/FileManager.h"
#include "base/TaskManager.h"

using namespace lss::base;

int main(int argc, const char** argv)
{
   // printf("hello world !\n");


    // 加载配置文件
    if (!configManager->LoadConfig("../config/config.json"))
    {
        std::cerr << "Load config file failed." << std::endl;
        
        return -1;
    }

    ConfigPtr config = configManager->GetConfig();
    LogInfoPtr log_info = config->GetLogInfo();
    std::cout << "Log level: " << log_info->level 
    << ", path: " << log_info->path 
    << ", name: " << log_info->name 
    << std::endl;

    FileLogPtr log = fileManager->GetFileLog(log_info->path + log_info->name);
    if (!log)
    {
        std::cerr << "Cannot open log, exit." << std::endl;
        
        return -1;
    }

    log->SetRotate(log_info->rotate_type);

    g_logger = new Logger(log);
    g_logger->SetLogLevel(log_info->level);

    TaskPtr task = std::make_shared<Task> ([](const TaskPtr &task)
                                        {
                                            fileManager->OnCheck();
                                            task->Restart();
                                        }, 
                                        1000);

    taskManager->Add(task);

    while (1)
    {
        taskManager->OnWork();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}