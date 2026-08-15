#pragma once
//预编译头
#include "common/前置头文件包含.h"

namespace engine
{
    //日志系统
    class Log
    {
    private:
        //输出流别名
        using Stream = std::shared_ptr<std::ostream>;
        //文件输出流别名
        using FileStream = std::shared_ptr<std::ofstream>;
        //日志别名
        using LogState = std::pair<Stream,std::string>;
    public:
        //信息输出
        template<typename... Args>
        static void info(std::format_string<Args...> fmt, Args&&... args)
        {
            //调用文件输出重载 
            info({},fmt, std::forward<Args>(args)...);
        }
        //信息输出 —— 文件输出重载
        template<typename... Args>
        static void info(const std::string& file_name,std::format_string<Args...> fmt, Args&&... args)
        {
            //构建日志
            LogState buffer = log_build(file_name,fmt, std::forward<Args>(args)...);
            //输出消息
            output(buffer.first, buffer.second, "INFO");
        }

        //警告输出
        template<typename... Args>
        static void warn(std::format_string<Args...> fmt, Args&&... args)
        {
            //调用文件输出重载
            warn({}, fmt, std::forward<Args>(args)...);
        }
        //警告输出 —— 文件输出重载
        template<typename... Args>
        static void warn(const std::string& file_name,std::format_string<Args...> fmt, 
            Args&&... args)
        {
            //构建日志
            LogState buffer = log_build(file_name, fmt, std::forward<Args>(args)...);
            //输出消息
            output(buffer.first, buffer.second, "WARN");
        }

        //错误输出
        template<typename... Args>
        static void error(std::format_string<Args...> fmt, Args&&... args)
        {
            //调用文件输出重载
            error({}, fmt, std::forward<Args>(args)...);
        }
        //错误输出 —— 文件输出重载
        template<typename... Args>
        static void error(const std::string& file_name, std::format_string<Args...> fmt,
            Args&&... args)
        {
            //构建日志
            LogState buffer = log_build(file_name, fmt, std::forward<Args>(args)...);
            //输出消息
            output(buffer.first, buffer.second, "ERROR");
        }

        //调试输出 
        template<typename... Args>
        static void debug(std::format_string<Args...> fmt, Args&&... args)
        {
            //调用文件输出重载
            debug({}, fmt, std::forward<Args>(args)...);
        }
        //调试输出 —— 文件输出重载
        template<typename... Args>
        static void debug(const std::string& file_name, std::format_string<Args...> fmt,
            Args&&... args)
        {
            //构建日志
            LogState buffer = log_build(file_name, fmt, std::forward<Args>(args)...);
            //输出消息
            output(buffer.first, buffer.second, "DEBUG");
        }
    private:
        //输出流流控制
        static Stream stream_charge(const std::string& file_name)
        {
            //输出流
            Stream stream;

            //若未指定输出文件名
            if (file_name.empty())
                stream = std::shared_ptr<std::ostream>(&std::cout, [](std::ostream*) {});
            //若指定输出文件名
            else
            {
                //检查该文件名是否已创建流
                auto it = stream_set.find(file_name);
                //若流已经存在则指针指向该流
                if (it != stream_set.end())
                    stream = it->second;
                //若流不存在则创建新流
                else
                {
                    //分配流内存
                    stream.reset(new(std::nothrow) std::ofstream(file_name));
                    //若内存分配失败
                    if (!stream)
                    {
                        //切换输出流
                        stream = std::shared_ptr<std::ostream>(&std::cout, [](std::ostream*) {});
                        //输出信息
                        *stream << "[FATAL]" << "内存不足\n已切换至控制台输出\n";
                    }
                    else
                    {
                        //构造文件流指针
                        FileStream filestream = std::static_pointer_cast<std::ofstream>(stream);
                        //若文件打开成功则记录该流
                        if(filestream->is_open())
                            stream_set.insert({ file_name,filestream });
                        //若文件打开失败
                        else
                        {
                            //切换输出流
                            stream = std::shared_ptr<std::ostream>(&std::cout, [](std::ostream*) {});
                            //输出信息
                            *stream << "[ERROR]" << "文件打开失败\n已切换至控制台输出\n";
                        }
                    }
                }
            }

            //返回输出流
            return stream;
        }
        //日志构建
        template<typename... Args>
        static LogState log_build(const std::string& file_name,
            std::format_string<Args...> fmt, Args&&... args)
        {
            //日志缓冲
            LogState buffer{};
            //控制输出流
            buffer.first = stream_charge(file_name);
            //格式化输出内容
            buffer.second = std::format(fmt, std::forward<Args>(args)...);
            //返回日志
            return buffer;
        }
        //日志输出
        static void output(Stream stream,const std::string& output, const std::string& type)
        {
            //构造消息类型格式
            const std::string msg_type = std::format("[{}]", type);
            //输出消息
            *stream << msg_type << output << std::endl;
        }

        //输出流集合
        inline static std::unordered_map<std::string,FileStream> stream_set;
    };
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