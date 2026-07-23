#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"

//游戏引擎命名空间
namespace Game_Engine
{
	//抽象事件结构体
	struct event
	{
		//目标接收者
		string target_module{};
		//事件大类
		string category;
		//类内标签
		string tag;

		//使用默认等于运算符
		bool operator==(const event&) const = default;
		//析构函数保证该结构体不可创建
		virtual ~event() = 0;

	};

	//通用配置结构体
	struct config_event : public event
	{
		//配置包
		json config;
	};
}

namespace std
{
	template<>
	struct hash<Game_Engine::event>
	{   // 注意你的 event 全名
		size_t operator()(const Game_Engine::event& evt) const {
			// 使用简单的组合哈希
			size_t h1 = hash<string>{}(evt.category);
			size_t h2 = hash<string>{}(evt.tag);
			size_t h3 = hash<string>{}(evt.target_module);
			// 混合三个哈希值（经典的 boost::hash_combine 原理）
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
}
