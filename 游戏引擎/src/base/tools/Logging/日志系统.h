#pragma once
#include "common/前置头文件包含.h"

namespace Log
{
	//日志输出
    template<typename... Args>
    void out(std::format_string<Args...> fmt, Args&&... args) 
    {
        //使用std::format进行编译期类型安全检查并格式化
        std::string msg = std::format(fmt, std::forward<Args>(args)...);

        //格式化字符串输出至控制台
        std::cout << "[INFO] " << msg << std::endl;
    }
}