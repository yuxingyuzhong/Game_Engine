#pragma once
//预编译头
#include "common/前置头文件包含.h"
//获取预定义通信结构体(用于函数返回值)
#include"src/core/space/Quadtree/四叉树通信结构体.h"

//引擎命名空间
namespace engine
{
	//四叉树管理器前向声明
	template <typename T>
	class Quadtree_Manager;
	//四叉树前向声明
	template <typename T>
	class Quadtree;

	//四叉树记录结构体
	template<typename T>
	struct tree_record
	{
	private:
		//四叉树指针
		Quadtree<T>* tree = nullptr;
	public:
		//四叉树管理器友元声明
		friend Quadtree_Manager<T>;
		//四叉树根节点坐标
		coord2D_double root{ 0.5,0.5 };
		//四叉树大小
		uint16_t size = 256;
	};

	//四叉树管理器设置结构体
	struct tree_manager_settings
	{
		//最小区块单元大小
		uint64_t block_size = 16;
		//四叉树大小上限
		uint64_t max_tree_size = 65536;
		//四叉树大小下限
		uint64_t min_tree_size = 256;
		//缓存启用状态
		bool is_cache_enabled = true;
		//缓存启用阈值
		uint64_t cache_active_threshold = 32;
		//缓存条目上限
		uint64_t max_cache_records = 16;
	};
}
