#pragma once
//预编译头
#include "common/前置头文件包含.h"

namespace engine
{
    //时钟别名
    using Clock = std::chrono::steady_clock;
    //时间戳别名
    using TimePoint = Clock::time_point;
    //时间段别名
    using Duration = Clock::duration;
}