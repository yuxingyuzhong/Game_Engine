#pragma once
//预编译头
#include "common/前置头文件包含.h"
//获取日志系统
#include "src/tools/Non_GUI/logging/日志系统.h"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <limits.h>
#endif

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
					// Windows：使用宽字符 API 获取可执行文件路径，支持中文等 Unicode
					std::wstring buffer(32767, L'\0');
					DWORD size = GetModuleFileNameW(nullptr, buffer.data(),
						static_cast<DWORD>(buffer.size()));
					if (size == 0) {
						throw std::runtime_error("GetModuleFileNameW failed");
					}
					if (size >= buffer.size()) {
						buffer.resize(size + 1);
						size = GetModuleFileNameW(nullptr, buffer.data(),
							static_cast<DWORD>(buffer.size()));
						if (size == 0) {
							throw std::runtime_error("GetModuleFileNameW failed after resize");
						}
					}
					buffer.resize(size);
					exe_path = std::filesystem::path(buffer);

			#elif defined(__linux__)
					// Linux：读取 /proc/self/exe 符号链接
					std::string buffer(PATH_MAX, '\0');
					ssize_t len;
					while ((len = readlink("/proc/self/exe", buffer.data(), buffer.size()))
						== static_cast<ssize_t>(buffer.size())) {
						buffer.resize(buffer.size() * 2);
					}
					if (len == -1) {
						throw std::runtime_error("readlink /proc/self/exe failed");
					}
					buffer.resize(len);
					exe_path = std::filesystem::path(buffer);

			#elif defined(__APPLE__)
					// macOS：使用 _NSGetExecutablePath，先获取所需大小
					uint32_t size = 0;
					_NSGetExecutablePath(nullptr, &size);
					if (size == 0) {
						throw std::runtime_error("_NSGetExecutablePath failed to get size");
					}
					std::string buffer(size, '\0');
					if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
						throw std::runtime_error("_NSGetExecutablePath failed");
					}
					exe_path = std::filesystem::path(buffer);

			#else
			#error "Unsupported platform"
			#endif

					exe_dir = exe_path.parent_path();
		}
	public:
		//获取可执行文件路径
		static const std::filesystem::path& exe_path_get(void)
		{
			//若未存储可执行文件路径
			if (exe_path.empty()) 
			{
				try 
				{
					exe_path_store();
				}
				catch (const std::exception& e) 
				{
					Log::error("错误: {}",e.what());
					Log::error("已回退到工作目录");
					//回退到当前工作目录
					exe_path = std::filesystem::current_path();
					exe_dir = exe_path.parent_path();
				}
			}
			//返回可执行文件路径
			return exe_path;
		}
		//获取可执行文件目录
		static const std::filesystem::path& exe_dir_get(void)
		{
			//若未存储可执行文件路径
			if (exe_path.empty())
			{
				try
				{
					exe_path_store();
				}
				catch (const std::exception& e)
				{
					Log::error("错误: {}", e.what());
					Log::error("已回退到工作目录");
					//回退到当前工作目录
					exe_path = std::filesystem::current_path();
					exe_dir = exe_path.parent_path();
				}
			}
			//返回可执行文件目录
			return exe_dir;
		}
		//绝对路径获取 —— 路径重载
		static std::filesystem::path absolute_path_get(const std::filesystem::path& path)
		{
			return exe_dir_get() / path;
		}
		//绝对路径获取 —— 字符串重载
		static std::filesystem::path absolute_path_get(const std::string& path)
		{
			//转化为可用字符串格式
			//要求string编码格式为UTF-8
			std::u8string u8_path(reinterpret_cast<const char8_t*>(path.data()), path.size());
			//返回拼接路径
			return exe_dir_get() / u8_path;
		}
	private:
		//可执行文件路径
		inline static std::filesystem::path exe_path{};
		//可执行文件目录
		inline static std::filesystem::path exe_dir{};
	};
}