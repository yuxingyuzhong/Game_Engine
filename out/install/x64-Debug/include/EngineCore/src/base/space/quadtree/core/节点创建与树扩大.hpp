#pragma once
#include "src/base/space/quadtree/函数预声明.h"

//子节点递归
template <typename T>
bool Quadtree<T>::child_node_recur(Node*& this_node, const int& direct, const Node_type& mode)
{
	//若当前子节点为空
	//则递归指针指向子节点并分配内存
	if (this_node->ptr_child[direct] == nullptr)
	{
		this_node->ptr_child[direct] = new(nothrow) Node(mode);
		this_node = this_node->ptr_child[direct];
		//若内存分配失败则返回false
		if (this_node == nullptr)
			return false;
	}
	//若子节点不为空指针且该子节点为中间节点
	//则递归指针指向子节点
	else
		this_node = this_node->ptr_child[direct];

	//若无异常发生则返回true
	return true;
}

//四叉树扩大
template <typename T>
bool Quadtree<T>::tree_expand(void)
{
	//分配中间节点内存
	Node* ptr_NW_new = new(nothrow) Node(MIDDLE);
	Node* ptr_NE_new = new(nothrow) Node(MIDDLE);
	Node* ptr_SW_new = new(nothrow) Node(MIDDLE);
	Node* ptr_SE_new = new(nothrow) Node(MIDDLE);
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
