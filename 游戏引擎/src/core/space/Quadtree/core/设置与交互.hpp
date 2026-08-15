#pragma once
#include "../函数预声明.h"

//展开命名空间
namespace engine
{
	//构造函数
	template <typename T>
	Quadtree<T>::Quadtree(const uint64_t& size, const coord2D_double& root)
	{
		state.size = size;
		state.root = root;
	}

	//析构函数
	template <typename T>
	Quadtree<T>::~Quadtree(void)
	{
		//根节点递归总级数声明
		int max_level = 0;
		//递归总级数计算
		recur_level_calcu(max_level);

		//当前节点指针
		Node* ptr_now = &root;
		//子节点指针
		Node* ptr_next = ptr_now;

		for (int recur_direct = 0; recur_direct <= SE; recur_direct++)
		{
			//递归子节点
			child_node_recur(ptr_next, recur_direct, MIDDLE, false);
			//递归子函数
			unload(1, max_level, ptr_next);
			//还原当前节点指针
			ptr_next = ptr_now;
		}
	}

	//析构函数辅助函数
	template <typename T>
	void Quadtree<T>::unload(int now_level, const int& max_level, Node* ptr_now)
	{
		//若当前节点为空指针则直接返回
		if (ptr_now == nullptr)
			return;

		//若当前为最后一级节点
		if (now_level == max_level)
		{
			//若叶子节点不为指针类型
			if constexpr (!std::is_pointer_v<T>)
				ptr_now->leaf.~T();
			//释放指向该节点的指针
			delete ptr_now;
			//返回上级递归
			return;
		}

		//默认下级节点为中间节点
		Node_type type = MIDDLE;
		//若当前为最后一次递归寻址
		if (now_level == max_level - 1)
			type = LEAF;

		//子节点指针
		Node* ptr_next = ptr_now;

		for (int recur_direct = 0; recur_direct <= SE; recur_direct++)
		{
			//递归子节点
			child_node_recur(ptr_next, recur_direct, type, false);
			//递归子函数
			unload(now_level + 1, max_level, ptr_next);
			//还原当前节点指针
			ptr_next = ptr_now;
		}

		//删除中间节点指针
		delete ptr_now;
	}

	//最小区块单元大小设置
	template <typename T>
	void Quadtree<T>::set_block_size(const uint64_t& size)
	{
		state.block_size = size;
	}

	//四叉树边长上限设置
	template <typename T>
	void Quadtree<T>::set_max_size(const uint64_t& size)
	{
		state.max_size = size;
	}

	//四叉树回调管理方法设置
	template <typename T>
	void Quadtree<T>::set_callback_manage
	(const std::function<bool(coord2D_double root, coord2D_int target)>& cb)
	{
		//回调管理函数注册
		callback = cb;
	}

	//四叉树状态获取
	template <typename T>
	const tree_state& Quadtree<T>::tree_state_get(void)
	{
		return state;
	}

}

