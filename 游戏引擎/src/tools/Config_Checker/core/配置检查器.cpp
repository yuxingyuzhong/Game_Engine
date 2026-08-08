#include "src/tools/Config_Checker/局部命名空间使用.h"
#include "src/tools/Logging/日志系统.h"

//引擎命名空间
namespace engine
{
	//异常信息输出
	bool Config_Checker::error_out(error_code& error_info)
	{
		//若异常信息不存在
		if (!error_info)
			//返回异常未输出
			return false;
		//若异常信息存在
		else
		{
			//输出异常信息
			Log::info("{}",error_info);
			//清空异常信息
			error_info.clear();
			//返回异常已输出
			return true;
		}
	}

	//路径有效性检查 —— path重载
	bool Config_Checker::path_check(const std::filesystem::path& config_path)
	{
		//若未解析出有效路径
		if (config_path.begin() == config_path.end())
		{
			Log::info("路径无效");
			return false;
		}

		//异常信息记录
		error_code ec;
		//若访问路径不存在或发生系统错误
		if (!exists(config_path, ec))
		{
			Log::info("访问路径不存在");
			error_out(ec);
			return false;
		}
		//若访问路径非可读取文件
		if (!is_regular_file(config_path, ec))
		{
			Log::info("访问路径不可读取");
			error_out(ec);
			return false;
		}

		//若所有检查均通过
		return true;
	}

	//路径有效性检查 —— string重载
	bool Config_Checker::path_check(const std::string& config_path)
	{
		//转化为可用字符串格式
		//要求string编码格式为UTF-8
		u8string u8config_path(config_path.begin(), config_path.end());
		//转化为标准路径
		path suspect_path = u8config_path;
		//调用path重载
		return path_check(suspect_path);
	}
}