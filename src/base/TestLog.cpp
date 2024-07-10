#include "Logger.h"
#include "LogStream.h"

using namespace lss::base;

void TestLog()
{
    LOG_TRACE << " Test Trace !!! ";
    LOG_DEBUG << " Test Debug !!! ";
    LOG_INFO << " Test Info !!! ";
    LOG_WARNING << " Test Warning !!! ";
    LOG_ERROR << " Test Error !!! ";
}

int main(int argc, const char **argv)
{
    lss::base::g_logger = new Logger();

    lss::base::g_logger->SetLogLevel(kWarning);

    TestLog();    

    return 0;
}