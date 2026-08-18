#pragma once
//预编译头
#include "common/前置头文件包含.h"

namespace engine
{
	//引擎环境
	class Engine_Env
	{
	private:
		//存储可执行文件路径
		static void exe_path_store(void)
		{
			#if defined(_WIN32) || defined(_WIN64)
						// Windows：获取 exe 完整路径，返回其父目录
						char buf[MAX_PATH];
						GetModuleFileNameA(NULL, buf, MAX_PATH);
						//存储可执行文件路径
						exe_path = std::filesystem::path(buf);

			#elif defined(__linux__)
						// Linux：读取 /proc/self/exe 符号链接，返回其父目录
			            exe_path = std::filesystem::read_symlink("/proc/self/exe");

			#elif defined(__APPLE__)
						// macOS：获取可执行文件路径，返回其父目录
						char buf[PATH_MAX];
						uint32_t size = sizeof(buf);
						_NSGetExecutablePath(buf, &size);
						exe_path = std::filesystem::path(buf);
			#else
			#error "Unsupported platform"
			#endif

			//存储可执行文件目录
			exe_dir = exe_path.parent_path();
		}
	public:
		//获取可执行文件路径
		static const std::filesystem::path& exe_path_get(void)
		{
			//若未存储可执行文件路径
			if (exe_path.empty())
				exe_path_store();
			//返回可执行文件路径
			return exe_path;
		}
		//获取可执行文件目录
		static const std::filesystem::path& exe_dir_get(void)
		{
			//若未存储可执行文件目录
			if (exe_dir.empty())
				exe_path_store();
			//返回可执行文件目录
			return exe_dir;
		}
		//绝对路径获取 —— 路径重载
		inline std::filesystem::path absolute_path_get(const std::filesystem::path& path)
		{
			return exe_path_get() / path;
		}
		//绝对路径获取 —— 字符串重载
		inline std::filesystem::path absolute_path_get(const std::string& path)
		{
			//转化为可用字符串格式
			//要求string编码格式为UTF-8
			std::u8string u8_path(reinterpret_cast<const char8_t*>(path.data()), path.size());
			//返回拼接路径
			return exe_path_get() / u8_path;
		}
	private:
		//可执行文件路径
		inline static std::filesystem::path exe_path{};
		//可执行文件目录
		inline static std::filesystem::path exe_dir{};
	};
}