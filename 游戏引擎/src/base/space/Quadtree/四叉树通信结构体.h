#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"

//游戏引擎命名空间
namespace Game_Engine
{
	//四叉树管理器前向声明
	template <typename T>
	class Quadtree_Manager;
	//四叉树前向声明
	template <typename T>
	class Quadtree;

	//四叉树状态结构体
	struct tree_state
	{
		//为避免根节点中心偏移现象
		//故采用小数坐标
		//根节点坐标
		coord_double root = { 0.5,0.5 };
		//四叉树大小
		uint64_t size = 256;
		//四叉树大小上限
		uint64_t max_size = 9223372036854775808;//初始化2的63次方
		//最小区块单元大小
		uint64_t block_size = 16;
	};

	//四叉树输出信息结构体
	template<typename T>
	struct tree_block_data
	{
	public:
		//友元声明，允许四叉树管理器类访问私有成员
		friend class Quadtree_Manager<T>;
		//友元声明，允许四叉树类访问私有成员
		friend class Quadtree<T>;
		//友元声明，允许被包装对象访问私有成员
		friend T;

		//默认构造函数
		tree_block_data() {}
		//列表构造函数：直接接收坐标和 T* 指针
		tree_block_data(float x, float y, T* ptr)
		{
			node.X = x;
			node.Y = y;
			ptr_data = ptr;
		}
		//默认析构函数
		~tree_block_data() {}

	private:
		coord_double node = { 0.5f, 0.5f };  // 使用 0.5f 强调 float 类型
		T* ptr_data = nullptr;
	};
}
