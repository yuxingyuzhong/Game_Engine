#pragma once
#include "../函数预声明.h"

//展开命名空间
namespace engine
{
	//子节点递归
	template <typename T>
	bool Quadtree<T>::child_node_recur(Node*& this_node, const int& direct, const Node_type& type, bool stable)
	{
		//若当前子节点为空且为稳定查询模式
		//则为子节点分配内存
		if (this_node->ptr_child[direct] == nullptr && stable == true)
			this_node->ptr_child[direct] = new(std::nothrow) Node(type);

		//递归指针子节点
		this_node = this_node->ptr_child[direct];

		//若子节点为空则返回false
		if (this_node == nullptr)
			return false;
		//若无异常发生则返回true
		else
			return true;
	}

	//四叉树扩大
	template <typename T>
	bool Quadtree<T>::tree_expand(void)
	{
		//分配中间节点内存
		Node* ptr_NW_new = new(std::nothrow) Node(MIDDLE);
		Node* ptr_NE_new = new(std::nothrow) Node(MIDDLE);
		Node* ptr_SW_new = new(std::nothrow) Node(MIDDLE);
		Node* ptr_SE_new = new(std::nothrow) Node(MIDDLE);
		//若存在内存分配失败
		if (ptr_NW_new == nullptr ||
			ptr_NE_new == nullptr ||
			ptr_SW_new == nullptr ||
			ptr_SE_new == nullptr)
		{
			//释放所有内存
			delete ptr_NW_new;
			delete ptr_NE_new;
			delete ptr_SW_new;
			delete ptr_SE_new;
			//放弃四叉树扩大
			return false;
		}
		//将树原数据链接进中间节点
		//注：由于四叉树是原地扩大
		//所以原来根节点直接管辖的区块间多了层中间节点
		//而方向则在原来的方向上的反方向
		ptr_NW_new->ptr_child[SE] = (*this).root.ptr_child[NW];
		ptr_NE_new->ptr_child[SW] = (*this).root.ptr_child[NE];
		ptr_SW_new->ptr_child[NE] = (*this).root.ptr_child[SW];
		ptr_SE_new->ptr_child[NW] = (*this).root.ptr_child[SE];
		//将中间节点链接进根节点
		(*this).root.ptr_child[NW] = ptr_NW_new;
		(*this).root.ptr_child[NE] = ptr_NE_new;
		(*this).root.ptr_child[SW] = ptr_SW_new;
		(*this).root.ptr_child[SE] = ptr_SE_new;
		//更新四叉树大小
		state.size *= 2;
		//返回扩大成功
		return true;
	}

}
