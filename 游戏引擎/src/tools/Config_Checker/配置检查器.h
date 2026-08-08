#pragma once
#include "common/前置头文件包含.h"
#include "src/tools/Logging/日志系统.h"

namespace engine
{
	//配置检查器
	class Config_Checker
	{
	private:
		//异常信息输出
		bool error_out(std::error_code& error_info);
	public:
		//字段有效性检查
		template<typename T>
		bool field_check(const nlohmann::json& config, const std::string& field)
		{
			//检查配置数据是否包含指定字段
			if (!config.contains(field))
			{
				Log::info("EffectComponet::未包含指定字段:");
				Log::info("{}", field);
				return false;
			}

			//检查指定字段是否为指定类型
			try
			{
				config[field].get<T>();
			}
			catch (const nlohmann::json::type_error&)
			{
				Log::info("Config_Checker::字段 {} 类型不匹配", field);
				return false;
			}

			//检查指定字段内容是否为空
			if (config[field].empty())
			{
				Log::info("EffectComponet::所需字段无内容");
				return false;
			}

			//若所有检查通过
			return true;
		}

		//路径有效性检查 —— path重载
		bool path_check(const std::filesystem::path& config_path);
		//路径有效性检查 —— string重载
		bool path_check(const std::string& config_path);

		//运算符重载
		template<typename T>
		bool operator()(const nlohmann::json& config, const std::string field)
		{
			return field_check<T>(config, field);
		}
		bool operator()(const std::filesystem::path& config_path)
		{
			return path_check(config_path);
		}
	};
}