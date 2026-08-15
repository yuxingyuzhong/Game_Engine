#pragma once
#include "../函数预声明.h"

//展开命名空间
namespace engine
{
	//单点查询可行性分析
	template <typename T>
	int Quadtree<T>::point_seekable_analyse(const coord2D_int& target)
	{
		/*函数逻辑：
				  0，代表分析已经结束，查找不可行
				  1，代表分析正在进行，查找可能可行
				  2，代表分析已经结束，查找可行
		*/

		//简化表示路径
		auto& root = state.root;
		//四叉树管理范围存储
		coord2D_range tree_range{};
		//计算四叉树管理范围
		manage_range_calcu(tree_range, state.root, state.size);

		//判断坐标大小是否超出树
		//若坐标大小超出树则进行下一步检测
		if (target.X < tree_range.left || target.X > tree_range.right
			|| target.Y > tree_range.up || target.Y < tree_range.down)
		{
			//若当前四叉树大小以及大于等于上限大小则查找不可行
			if (state.size >= state.max_size)
			{
				//若存在回调管理函数则报告上级
				if (callback)
					callback(root, target);
				//返回分析终止值
				return 0;
			}
			//若当前四叉树大小小于上限大小则寻址可能可行
			else if (state.size < state.max_size)
			{
				//若存在回调管理函数则请求扩大权限
				if (callback)
					//若扩大申请未通过则返回分析终止值
					if (!callback(root, target))
						return 0;

				//进行四叉树扩大操作
				//若四叉树扩大成功则进行下一步操作
				if (tree_expand())
				{
					//重新计算四叉树管理范围
					manage_range_calcu(tree_range, state.root, state.size);

					//重新比较四叉树是否已经包含待查找位置
					//若未包含则返回分析持续值
					if (target.X < tree_range.left || target.X > tree_range.right
						|| target.Y > tree_range.up || target.Y < tree_range.down)
						return 1;
					//若已包含则返回返回查找可行值
					else
						return 2;
				}
				//若未成功则直接返回分析终止值
				else
					return 0;
			}
		}
		//若坐标大小未超出树则直接返回查找可行值
		else
			return 2;

	}

	//范围查询可行性分析
	template <typename T>
	void Quadtree<T>::range_seekable_analyse(const coord2D_range& format_range, coord2D_range& seekable_range)
	{
		for (;;)
		{
			//计算四叉树当前查询范围
			manage_range_calcu(seekable_range, state.root, state.size);
			//获取是否扩大标记
			coord2D_double expand_register = seekable_range_calcu(format_range, seekable_range);

			//若返回坐标非树根节点坐标
			//则进行扩大(若存在管理层则进行申请)
			if (expand_register != state.root)
			{
				//若存在回调则进行扩大申请
				if (callback)
				{
					//若扩大申请通过通过则扩大
					if (callback(state.root, expand_register))
					{
						//若扩大失败则直接结束计算
						if (!tree_expand())
							break;
					}
					//若扩大申请未通过则直接结束循环
					else
						break;
				}
				//若不存在回调则直接进行扩大
				else if (state.size < state.max_size)
				{
					//若扩大失败则直接结束计算
					if (!tree_expand())
						break;
				}

				//若当前四叉树大小以及大于等于上限大小则直接结束计算
				if (state.size >= state.max_size)
					break;
			}
			//反之则直接退出
			else
				break;
		}
	}

	//递归栈操作
	template <typename T>
	void Quadtree<T>::recur_stack_operate(std::vector<recur_record>& recur_stack,
		Node*& ptr, coord2D_range& range, int& level,
		bool push_back)
	{
		//若为弹栈操作
		if (push_back == true)
			recur_stack.push_back({ ptr,range ,level });
		//若为压栈操作
		else
		{
			//简化表示路径
			auto& record = recur_stack.back();
			//获取记录指针
			ptr = record.node;
			//获取记录范围
			range = record.node_range;
			//获取记录递归级数
			level = record.recur_level;
			//弹出栈顶记录
			recur_stack.pop_back();
		}
	}

	//最小区块单元查找
	template <typename T>
	void Quadtree<T>::block_seek(tree_chunk_data<T>*& receiver, const coord2D_int& target, bool stable)
	{
		//四叉树上限上限临时存储
		int max_size = state.max_size;
		//最大检测次数存储
		int exam_time_max = 1;
		//计算最大检测次数
		for (; (max_size /= 2) / state.size > 1;)
			exam_time_max++;

		//循环检测查找是否可行
		//循环次数保证理想情况下四叉树可扩大到最大
		//额外次数保证可能存在的管理层知晓查询失败信息
		for (int check_time = 0; check_time < exam_time_max; check_time++)
		{
			//获取下一步分析方案
			int next_step = point_seekable_analyse(target);
			//若查找可行则直接结束检测
			if (next_step == 2)
				break;
			//若查找可能可行则继续
			else if (next_step == 1)
				continue;
			//若查找不可行则直接返回
			else if (next_step == 0)
				//返回给上层调用者
				return;
		}

		//根节点寻址总级数声明
		int recur_level_max = 0;
		//递归总级数计算
		recur_level_calcu(recur_level_max);

		//获取根节点指针
		Node* child_node = &root;
		//节点管理范围存储
		coord2D_range node_range{};
		//初始化为四叉树管理范围
		manage_range_calcu(node_range, state.root, state.size);
		//路径递归方向标记存储
		int recur_direct = 0;

		//开始区块检索
		for (int recur_level_now = 0; recur_level_now < recur_level_max; recur_level_now++)
		{
			//计算递归方向
			recur_direct_calcu(target, node_range, recur_direct);
			//递归子节点
			//若当前不为最后一级则创建中间节点
			if (recur_level_now < recur_level_max - 1)
				child_node_recur(child_node, recur_direct, MIDDLE, stable);
			//若当前为最后一级递归则创建叶子节点
			else if (recur_level_now == recur_level_max - 1)
				child_node_recur(child_node, recur_direct, LEAF, stable);

			//若内存分配失败则直接返回
			if (child_node == nullptr)
				return;

			//存储旧范围值
			coord2D_range old_range = node_range;
			//计算新范围值
			child_node_range_calcu(recur_direct, node_range, old_range);
		}

		//若指针为空则分配内存
		if (receiver == nullptr)
		{
			receiver = new(std::nothrow) tree_chunk_data<T>;
			//若内存分配失败则返回
			if (receiver == nullptr)
				return;
		}

		//记录查询结果
		receiver->ptr_data = &(child_node->leaf);
		receiver->node.X = (node_range.left + node_range.right) / 2.0f;
		receiver->node.Y = (node_range.down + node_range.up) / 2.0f;
	}

	//范围区块单元查找
	template <typename T>
	void Quadtree<T>::range_seek(std::vector<tree_chunk_data<T>*>& receiver, 
		const coord2D_range& target_range, bool stable)
	{
		//可查询范围存储
		coord2D_range seekable_range{};
		//格式化待查询范围存储
		coord2D_range format_range = target_range;
		//格式化待查询范围
		target_range_format(format_range, state.root, state.block_size);
		//分析获得可查询范围
		range_seekable_analyse(format_range, seekable_range);

		//根节点寻址总级数声明
		int recur_level_max = 0;
		//寻址总级数计算
		recur_level_calcu(recur_level_max);

		//矢量模拟堆栈
		std::vector<recur_record> stack{};
		//递归路径存储
		std::vector<int> recur_path(recur_level_max, NW);

		//父节点指针存储
		Node* parent_node = &root;
		//父节点管理范围存储
		coord2D_range parent_range{};
		//父节点初始化为四叉树管理范围
		manage_range_calcu(parent_range, state.root, state.size);
		//子节点管理范围存储
		coord2D_range child_range{};

		//递归查找子区块
		for (int recur_level_now = 0; recur_level_now < recur_level_max;)
		{
			//简化表示路径
			auto& recur_direct = recur_path[recur_level_now];
			//可递归子节点数量
			int recursive_num = 0;
			//可递归子节点方向记录
			bool is_direct_record = false;

			//寻找可查找子节点
			for (int now_direct = recur_direct; now_direct <= SE; now_direct++)
			{
				//计算子节点管理范围
				child_node_range_calcu(now_direct, child_range, parent_range);

				//若子节点包含待查找范围
				//无论全包含或者部分包含
				if (range_relation_get(seekable_range, child_range))
				{
					//记录可递归区块数目
					recursive_num++;
					//若尚未记录可递归方向则记录
					if (is_direct_record == false)
					{
						//记录递归方向
						recur_direct = now_direct;
						//设置标记位
						is_direct_record = true;
					}
				}
			}

			//若可查找子节点数目超过一
			//且当前不为叶子层级
			//则向栈存储信息
			if (recursive_num > 1 && recur_level_now != recur_level_max - 1)
				recur_stack_operate(stack, parent_node, parent_range,
					recur_level_now, true);

			//若当前不为最后一级递归
			//则下级节点为中间节点
			if (recur_level_now < recur_level_max - 1)
			{
				//弹栈操作标记位
				bool is_pop_back = true;

				//父节点递归
				//若递归失败则弹栈
				if (child_node_recur(parent_node, recur_direct, MIDDLE, stable))
				{
					//存储父节点范围
					coord2D_range old_range = parent_range;
					//更新父节点范围
					child_node_range_calcu(recur_direct, parent_range, old_range);
					//更新递归级数
					recur_level_now++;
					//标记无需弹栈
					is_pop_back = false;
				}

				//若当前层级尚有未查找方向
				if (recursive_num > 1)
				{
					//更新递归方向
					recur_direct++;
					//标记无需弹栈
					is_pop_back = false;
				}
				//若当前层级无未查找方向
				else
					//重置当前层级递归方向记录
					recur_direct = NW;

				//若弹栈操作为真且栈内元素为零则直接结束查找
				if (is_pop_back == true && stack.size() == 0)
					break;
				//若弹栈操作为真且栈内元素不为零则读取信息
				else if (is_pop_back == true && stack.size() != 0)
					recur_stack_operate(stack, parent_node, parent_range,
						recur_level_now, false);
			}
			//若当前为最后一级递归
			//则下级节点为叶子节点
			else if (recur_level_now == recur_level_max - 1)
			{
				//一次性取出当前节点下辖所有待取出叶子节点
				for (; recur_direct <= SE; recur_direct++)
				{
					//计算子节点范围
					child_node_range_calcu(recur_direct, child_range, parent_range);

					//若当前叶子节点未在可查询范围内则略过
					if (!range_relation_get(seekable_range, child_range))
						continue;

					//子节点指针存储
					Node* child_node = parent_node;
					//递归子节点
					//若递归失败则查找下一节点
					if (!child_node_recur(child_node, recur_direct, LEAF, stable))
						continue;
					//记录查询结果
					auto* new_data = new(std::nothrow) tree_chunk_data<T>
						((child_range.left + child_range.right) / 2.0f,
							(child_range.up + child_range.down) / 2.0f,
							&child_node->leaf);
					//若内存分配失败则直接返回
					if (new_data == nullptr)
						return;
					else
						receiver.push_back(new_data);
				}

				//重置当前层级递归方向记录
				recur_direct = NW;
				//若堆栈记录取用失败则结束查找进程
				if (stack.size() == 0)
					break;
				//反之则从堆栈中读取信息
				else
					recur_stack_operate(stack, parent_node,
						parent_range, recur_level_now, false);
			}
		}
	}

}
