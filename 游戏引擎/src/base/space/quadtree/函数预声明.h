#pragma once
#include "common/前置头文件包含.h"
#include "common/coord_types.h"
#include "四叉树通信结构体.h"

namespace engine
{
	template<typename T>
	//四叉树模板
	class Quadtree
	{
	private:
		
		// ———— 内部类型定义 ————

		//节点类型枚举
		enum Node_type
		{
			//中间节点
			MIDDLE,
			//叶子节点
			LEAF
		};
		//节点信息联合体
		union Node
		{
			Node(const Node_type& mode = MIDDLE)
			{
				//默认激活ptr_child成员并置空指针
				if (mode == MIDDLE)
					new (&ptr_child) Node* [4]();
				//激活leaf成员
				else if (mode == LEAF)
					new (&leaf) T();
			}
			~Node()
			{}

			//节点指针
			Node* ptr_child[4];
			//区块
			T leaf;

		}root;
		//查找区域分布情况枚举
		enum range_relation
		{
			PART_IN,
			NONE_IN
		};
		//节点递归方向枚举
		enum recur_direct { NW, NE, SW, SE };
		//节点递归记录结构体
		struct recur_record
		{
			//节点
			Node* node;
			//节点范围
			coord2D_range node_range{};
			//递归级别
			int recur_level = 0;
		};
		//四叉树状态记录
		tree_state state;
		//外界上级管理对象回调管理方法----四叉树扩大行为权限申请
		std::function<bool(const coord2D_double& root, const coord2D_int& target)> callback;

		// ———— 公开接口 ————
	public:
		//构造函数
		Quadtree(const uint64_t& size = 256, const coord2D_double& root = { 0.5,0.5 });
		//析构函数
		~Quadtree(void);
		//析构函数辅助函数
		void unload(int now_level, const int& max_level,Node* ptr_now);

		// ---- 设置 ----
		//最小区块单元大小设置
		void set_block_size(const uint64_t& size);
		//四叉树边长上限设置
		void set_max_size(const uint64_t& size);
		//四叉树回调管理方法设置
		void set_callback_manage
		(const std::function<bool(coord2D_double root, coord2D_int target)>& cb);

		// ---- 查询 ----
		//最小区块单元查找
		void block_seek(tree_block_data<T>*& receiver, const coord2D_int& target, bool stable);
		//范围区块单元查找
		void range_seek(std::vector<tree_block_data<T>*>& receiver, const coord2D_range& target_range, bool stable);

		// ---- 读取 ----
		//四叉树状态获取
		const tree_state& tree_state_get(void);

		// ---- 维护 ---- 
		//四叉树扩大
		bool tree_expand(void);

		// ———— 底层计算工具 ————
	private:
		//递归级数计算
		void recur_level_calcu(int& address_series);

		//递归方向计算
		void recur_direct_calcu(const coord2D_int& target, const coord2D_range& node, int& recur_direct);

		//子节点范围计算
		void child_node_range_calcu(const int& recur_direct, coord2D_range& child_range,
			const coord2D_range& parent_range);
	public:
		//四叉树管理范围计算
		void manage_range_calcu(coord2D_range& receiver,const coord2D_double& root,const uint64_t tree_size);

		//待查询范围格式化
		void target_range_format(coord2D_range& target_range,const coord2D_double& root, const uint64_t block_size);

		//可查询范围计算
		coord2D_double seekable_range_calcu(const coord2D_range& target_range, coord2D_range& seekable_range);
	private:
		//查询范围关系获取
		bool range_relation_get(const coord2D_range& target_range, const coord2D_range& node_range);

		// ———— 结构维护 ————
	private:
		//子节点递归
		bool child_node_recur(Node*& this_node, const int& direct, const Node_type& type, bool stable);

		// ———— 查询前置支撑 ————
	private:
		//单点查询可行性分析
		int point_seekable_analyse(const coord2D_int& target);

		//范围查询可行性分析
		void range_seekable_analyse(const coord2D_range& format_range, coord2D_range& seekable_range);

		//递归栈操作
		void recur_stack_operate(std::vector<recur_record>& recur_stack,
			Node*& ptr, coord2D_range& range, int& level,
			bool push_back);

	};
}

//使用四叉树
using engine::Quadtree;