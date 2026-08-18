#pragma once
//预编译头
#include "common/前置头文件包含.h"
//获取日志系统
#include "src/tools/Non_GUI/Logging/日志系统.h"

namespace engine
{
	//配置检查器
	class Config_Checker
	{
	private:
		//异常信息输出
		bool error_out(std::error_code& error_info)
        {
            //若异常信息不存在
            if (!error_info)
                //返回异常未输出
                return false;
            //若异常信息存在
            else
            {
                //输出异常信息
                Log::info("{}", error_info);
                //清空异常信息
                error_info.clear();
                //返回异常已输出
                return true;
            }
        }
	public:
        //字段有效性检查
        template<typename T>
        bool field_check(const nlohmann::json& config, const std::string& field)
        {
            //字段存在性检查
            if (!config.contains(field)) 
            {
                Log::warn("Config_Checker::未包含指定字段: {}", field);
                return false;
            }

            //萃取整数类形
            if constexpr (std::is_integral_v<T>) 
            {
                //匹配所有整数类型
                if (!config[field].is_number_integer()) 
                {
                    Log::info("Config_Checker::字段 {} 非整数格式", field);
                    return false;
                }
            }
            //萃取浮点类型
            else if constexpr (std::is_floating_point_v<T>) 
            {
                //匹配所有浮点类型
                if (!config[field].is_number_float()) 
                {
                    Log::info("Config_Checker::字段 {} 非浮点数格式", field);
                    return false;
                }
            }
            // 萃取布尔类型
            if constexpr (std::is_same_v<T, bool>)
            {
                if (!config[field].is_boolean())
                {
                    Log::info("Config_Checker::字段 {} 非布尔格式", field);
                    return false;
                }
            }
            //若为其他类型
            else 
            {
                //匹配容器类型
                try 
                {
                    config[field].get<T>();
                }
                catch (const nlohmann::json::type_error&)
                {
                    Log::info("Config_Checker::字段 {} 类型不匹配", field);
                    return false;
                }

                //非空检查
                if (config[field].empty())
                {
                    Log::info("Config_Checker::字段 {} 内容为空", field);
                    return false;
                }
            }

            return true;
        }
		//路径有效性检查 —— path重载
		bool path_check(const std::filesystem::path& config_path)
        {
            //若未解析出有效路径
            if (config_path.begin() == config_path.end())
            {
                Log::info("路径无效");
                return false;
            }

            //异常信息记录
            std::error_code ec;
            //若访问路径不存在或发生系统错误
            if (!std::filesystem::exists(config_path, ec))
            {
                Log::info("访问路径不存在");
                error_out(ec);
                return false;
            }
            //若访问路径非可读取文件
            if (!std::filesystem::is_regular_file(config_path, ec))
            {
                Log::info("访问路径不可读取");
                error_out(ec);
                return false;
            }

            //若所有检查均通过
            return true;
        }
		//路径有效性检查 —— string重载
		bool path_check(const std::string& config_path)
        {
            //转化为可用字符串格式
            //要求string编码格式为UTF-8
            std::u8string u8config_path(config_path.begin(), config_path.end());
            //转化为标准路径
            std::filesystem::path suspect_path = u8config_path;
            //调用path重载
            return path_check(suspect_path);
        }
	};
}