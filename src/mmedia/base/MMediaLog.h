#pragma once
#include <iostream>
#include "base/LogStream.h"

using namespace lss::base;

#define RTMP_DEBUG_ON 1

#ifdef RTMP_DEBUG_ON
#define RTMP_TRACE std::cout << "\n"
#define RTMP_DEBUG LOG_DEBUG
#define RTMP_INFO LOG_INFO
#elif
#define RTMP_TRACE if(0) LOG_TRACE
#define RTMP_DEBUG if(0) LOG_DEBUG
#define RTMP_INFO if(0) LOG_INFO
#endif

#define RTMP_WARN LOG_WARN
#define RTMP_ERROR LOG_ERROR