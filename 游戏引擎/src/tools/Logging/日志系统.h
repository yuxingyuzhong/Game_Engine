#pragma once
#include "common/前置头文件包含.h"

namespace Log
{
	//日志输出
    template<typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args) 
    {
        //使用std::format进行编译期类型安全检查并格式化
        std::string msg = std::format(fmt, std::forward<Args>(args)...);

        //格式化字符串输出至控制台
        std::cout << "[INFO] " << msg << std::endl;
    }
}

namespace std
{
    template <>
    struct std::formatter<std::error_code> {
        // 简单实现：忽略格式说明符，直接输出错误码值和消息
        constexpr auto parse(format_parse_context& ctx) {
            return ctx.begin(); // 接受任何格式，不做特殊解析
        }

        auto format(const std::error_code& ec, format_context& ctx) const {
            // 注意：message() 可能抛出 bad_alloc，这里简单处理
            return std::format_to(ctx.out(), "[{}] {}", ec.value(), ec.message());
        }
    };
}