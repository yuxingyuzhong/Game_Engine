#pragma once
#include "src/base/space/quadtree/函数预声明.h"

//构造函数
template <typename T>
Quadtree<T>::Quadtree(const uint64_t& size, const coord_double& root)
{
	state.size = size;
	state.root = root;
}

//析构函数
template <typename T>
Quadtree<T>::~Quadtree(void)
{
	//根节点递归总级数声明
	int recur_level_max = 0;
	//递归总级数计算
	recur_level_calcu(recur_level_max);
	//递归路径存储
	vector<int> recur_path(recur_level_max, NW);

	//外层循环控制叶节点寻址次数
	//当最上层路径已经超出范围时即为寻址结束
	//因为此时会直接访问未知内存
	for (int recur_level_now = 0; recur_path[0] <= SE;)
	{
		//当前递归级数重置
		recur_level_now = 0;

		//内层寻址循环
		//从根节点开始遍历
		for (auto* ptr_now = &root;; recur_level_now++)
		{
			//简化表示路径
			auto& recur_direct = recur_path[recur_level_now];

			//若当前节点的子节点指针不为空
			if (ptr_now->ptr_child[recur_direct] != nullptr)
			{
				//若当前为最后一次递归寻址
				if (recur_level_now == recur_level_max - 1)
				{
					//记录下级节点指针
					auto*& ptr_next = ptr_now->ptr_child[recur_direct];
					//调用模板参数析构函数
					ptr_next->leaf.~T();
					//释放叶子节点内存
					delete ptr_next;
					//指针置空
					ptr_next = nullptr;

					//更新递归路径
					recur_direct++;
					//若本中间节点管理的叶子节点已经遍历完毕
					if (recur_direct > SE)
					{
						//则释放当前中间节点
						delete ptr_now;
						//指针置空
						ptr_now = nullptr;
					}

					//结束内层循环
					break;
				}
				//若当前不为最后一次递归寻址
				else
					ptr_now = ptr_now->ptr_child[recur_direct];
			}
			//若当前节点的子节点指针为空
			else if (ptr_now->ptr_child[recur_direct] == nullptr)
			{
				//更新递归路径
				recur_direct++;

				//若本中间节点管理的子节点已经遍历完毕
				if (recur_direct > SE)
				{
					//则释放当前中间节点
					if (ptr_now != &root)
						delete ptr_now;
					//指针置空
					ptr_now = nullptr;
				}
				//结束内层循环
				break;
			}
		}

		//递归路径额外校准
		for (int cali_time = recur_level_max - 1; cali_time > 0; cali_time--)
		{
			//从最后一级递归开始逐级检测递归路径
			if (recur_path[cali_time] > SE)
			{
				//若当前级数递归已经完毕则重置递归路径
				recur_path[cali_time] = NW;
				//更新上级递归路径
				recur_path[cali_time - 1] += 1;
			}
		}
	}
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
(const function<bool(coord_double root, coord_int target)>& cb)
{
	//回调管理函数注册
	callback = cb;
}
