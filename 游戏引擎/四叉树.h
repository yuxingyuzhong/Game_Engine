#ifndef THIS_QUADTREE
#define THIS_QUADTREE
#include "前置头文件包含.h"
#include "公共命名空间使用.h"

//使用四叉树输出结构体
using engine::tree_block_data;

namespace Game_Engine
{
	template<typename T>
	//四叉树模板
	class Quadtree
	{
	private:
		//Node联合体前向声明
		union Node;
		//节点处理枚举
		enum Node_process_mode
		{
			MIDDLE,
			LEAF
		};
		//查找区域分布情况枚举
		enum target_distr_case
		{
			ALL_IN,
			PART_IN,
			NONE_IN
		};
		//节点递归方向枚举
		enum recur_direct { NW, NE, SW, SE };
		//节点递归记录结构体
		struct recur_record
		{
			Node* node;
			coord_range node_range{};
			int recur_level = 0;
		};
	private:
		//节点信息结构体
		union Node
		{
			Node(const Node_process_mode& mode = MIDDLE)
			{
				//默认激活 ptr_child 成员，并将四个指针置空
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
		//树信息结构体
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
			uint16_t block_size = 16;

		}state;
		//外界上级管理对象回调管理方法----四叉树扩大行为权限申请
		function<bool(const coord_double& root, const coord_int& target)> callback;

	public:
		//构造函数
		Quadtree(const uint64_t& size = 256, const coord_double& root = { 0.5,0.5 })
		{
			state.size = size;
			state.root = root;
		}
		//析构函数
		~Quadtree(void)
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
	public:
		//最小区块单元大小设置
		void set_block_size(const uint64_t& size)
		{
			state.block_size = size;
		}
		//四叉树边长上限设置
		void set_max_size(const uint64_t& size)
		{
			state.max_size = size;
		}
		//四叉树回调管理方法设置
		void set_callback_manage
		(const function<bool(coord_double root, coord_int target)>& cb)
		{
			//回调管理函数注册
			callback = cb;
		}
	public:
		//最小区块单元查找
		void block_seek(tree_block_data<T>*& receiver, const coord_int& target)
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
				int next_step = check_seekable(target);
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
			coord_range node_range{};
			//初始化为四叉树管理范围
			manage_range_calcu(node_range);
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
					child_node_recur(child_node, recur_direct, MIDDLE);
				//若当前为最后一级递归则创建叶子节点
				else if (recur_level_now == recur_level_max - 1)
					child_node_recur(child_node, recur_direct, LEAF);

				//若内存分配失败则直接返回
				if (child_node == nullptr)
					return;

				//存储旧范围值
				coord_range old_range = node_range;
				//计算新范围值
				child_node_range_calcu(recur_direct, node_range, old_range);
			}

			//若指针为空则分配内存
			if (receiver == nullptr)
			{
				receiver = new(nothrow) tree_block_data<T>;
				//若内存分配失败则返回
				if (receiver == nullptr)
					return;
			}

			//记录查询结果
			receiver->ptr_data = &(child_node->leaf);
			// 修正后（使用浮点除法）
			receiver->node.X = (node_range.left + node_range.right) / 2.0f;
			receiver->node.Y = (node_range.down + node_range.up) / 2.0f;
		}
		//范围区块单元查找
		void range_seek(vector<tree_block_data<T>*>& receiver, const coord_range& target_range)
		{
			//可查询范围存储
			coord_range seekable_range{};
			//格式化待查询范围存储
			coord_range format_range = target_range;
			//格式化待查询范围
			target_range_format(format_range);
			//循环计算可查询范围
			for (;;)
			{
				//是否扩大标记
				coord_double coord_sign = seekable_range_calcu(seekable_range, format_range);
				//若返回坐标非树根节点坐标则进行扩大(若存在管理层则进行申请)
				if (coord_sign != state.root)
				{
					//若存在回调则进行扩大申请
					if (callback)
					{
						//若扩大申请通过通过则扩大
						if (callback(state.root, coord_sign))
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
						//若扩大失败则直接结束计算
						if (!tree_expand())
							break;
					//若当前四叉树大小以及大于等于上限大小则直接结束计算
						else if (state.size >= state.max_size)
							break;
				}
				//反之则直接退出
				else
					break;
			}

			//根节点寻址总级数声明
			int recur_level_max = 0;
			//寻址总级数计算
			recur_level_calcu(recur_level_max);

			//矢量模拟堆栈
			vector<recur_record> stack{};
			//递归路径存储
			vector<int> recur_path(recur_level_max, NW);
			//父节点指针存储
			Node* parent_node = &root;
			//父节点管理范围存储
			coord_range parent_range{};
			//子节点管理范围存储
			coord_range child_range{};
			//初始化为四叉树管理范围
			manage_range_calcu(parent_range);

			//递归查找子区块
			for (int recur_level_now = 0; recur_level_now < recur_level_max;)
			{
				//简化表示路径
				auto& recur_direct = recur_path[recur_level_now];
				//可查找子节点数目存储
				int seekable_num = 0;
				//可查找子节点方向记录标记
				bool is_direct_record = false;

				//寻找可查找子节点
				for (int now_direct = recur_direct; now_direct <= SE; now_direct++)
				{
					//计算子节点管理范围
					child_node_range_calcu(now_direct, child_range, parent_range);

					//若子节点全包含待查找范围
					if (target_range_compare(seekable_range, child_range) == ALL_IN)
					{
						//记录可递归区块数目
						seekable_num++;
						//记录可递归方向
						recur_direct = now_direct;
						//直接结束循环
						break;
					}
					//若子节点部分包含待查找范围
					else if (target_range_compare(seekable_range, child_range) == PART_IN)
					{
						//记录可递归区块数目
						seekable_num++;
						//若尚未记录可递归方向则记录
						if (is_direct_record == false)
						{
							recur_direct = now_direct;
							//设置标记位
							is_direct_record = true;
						}
					}
				}

				//若可查找子节点数目超过一
				//且当前不为叶子层级
				//则向栈存储信息
				if (seekable_num > 1 && recur_level_now != recur_level_max - 1)
					stack.push_back({ parent_node,parent_range ,recur_level_now });

				//若当前不为最后一级递归
				//则下级节点为中间节点
				if (recur_level_now < recur_level_max - 1)
				{
					//父节点递归
					//若递归失败则结束查找进程
					if (!child_node_recur(parent_node, recur_direct, MIDDLE))
						break;
					//更新父节点范围
					coord_range old_parent_range = parent_range;
					child_node_range_calcu(recur_direct, parent_range, old_parent_range);
					//若当前层级尚有未查找方向
					if (seekable_num > 1)
						//更新递归方向
						recur_direct++;
					//若当前层级无未查找方向
					else
						//重置当前层级递归方向记录
						recur_direct = NW;
					//更新递归级数
					recur_level_now++;
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
						if (target_range_compare(seekable_range, child_range) == NONE_IN)
							continue;

						//子节点指针存储
						Node* child_node = parent_node;
						//递归子节点
						//若递归失败则结束查找进程
						if (!child_node_recur(child_node, recur_direct, LEAF))
							break;
						//记录查询结果
						auto* new_data = new(nothrow) tree_block_data<T>
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
					{
						//简化表示路径
						auto& record = stack.back();
						//获取记录指针
						parent_node = record.node;
						//获取记录范围
						parent_range = record.node_range;
						//获取记录递归级数
						recur_level_now = record.recur_level;
						//弹出栈顶记录
						stack.pop_back();
					}
				}
			}
		}
	private:
		//递归级数计算
		void recur_level_calcu(int& address_series)
		{
			//寻址总级数计算
			for (uint64_t tree_size = state.size; (tree_size /= 2) >= state.block_size;)
				address_series++;

			//PS:从根节点开始向下递归时
			   //若当前指针已指向最小区块单元大小的区域
			   //此时已经为该指针分配内存
			   //递归结束
		}

		//递归方向计算
		void recur_direct_calcu(const coord_int& target, const coord_range& node, int& recur_direct)
		{
			//因节点坐标为浮点坐标
			//而查找坐标为整数坐标
			//因而不存在坐标相等情况

			//计算节点根坐标
			coord_double node_coord = {};
			node_coord.X = (node.left + node.right) / 2.0f;
			node_coord.Y = (node.down + node.up) / 2.0f;

			//西北方向
			if (target.X < node_coord.X && target.Y > node_coord.Y)
				recur_direct = NW;
			//东北方向
			else if (target.X > node_coord.X && target.Y > node_coord.Y)
				recur_direct = NE;
			//西南方向
			else if (target.X < node_coord.X && target.Y < node_coord.Y)
				recur_direct = SW;
			//东南方向
			else if (target.X > node_coord.X && target.Y < node_coord.Y)
				recur_direct = SE;
		}

		//子节点范围计算
		void child_node_range_calcu(const int& recur_direct, coord_range& child_range,
			const coord_range& parent_range)
		{
			//若子节点向西北递归
			if (recur_direct == NW)
			{
				child_range.left = parent_range.left;
				child_range.right = (parent_range.right + parent_range.left) / 2;
				child_range.up = parent_range.up;
				child_range.down = (parent_range.up + parent_range.down) / 2 + 1;
			}
			//若子节点向东北递归
			else if (recur_direct == NE)
			{
				child_range.left = (parent_range.right + parent_range.left) / 2 + 1;
				child_range.right = parent_range.right;
				child_range.up = parent_range.up;
				child_range.down = (parent_range.up + parent_range.down) / 2 + 1;
			}
			//若子节点向西南递归
			else if (recur_direct == SW)
			{
				child_range.left = parent_range.left;
				child_range.right = (parent_range.right + parent_range.left) / 2;
				child_range.up = (parent_range.up + parent_range.down) / 2;
				child_range.down = parent_range.down;
			}
			//若子节点向东南递归
			else if (recur_direct == SE)
			{
				child_range.left = (parent_range.right + parent_range.left) / 2 + 1;
				child_range.right = parent_range.right;
				child_range.up = (parent_range.up + parent_range.down) / 2;
				child_range.down = parent_range.down;
			}
		}

		//四叉树管理范围计算
		void manage_range_calcu(coord_range& receiver)
		{
			//简化表示路径
			auto& root = state.root;
			auto& size = state.size;

			//四叉树中心均为浮点坐标
			//存在初始0.5偏移需先行将其抵消
			receiver.left = (root.X - 0.5) - (size / 2 - 1);
			receiver.right = (root.X + 0.5) + (size / 2 - 1);
			receiver.down = (root.Y - 0.5) - (size / 2 - 1);
			receiver.up = (root.Y + 0.5) + (size / 2 - 1);
		}

		//待查询范围格式化
		void target_range_format(coord_range& target_range)
		{
			//简化表示路径
			auto& root = state.root;
			auto& block_size = state.block_size;

			//格式化目的:保证待查询范围严格包含内部区块

			// 安全取模（处理负数）
			auto mod = [](int a, int b) { int r = a % b; return r < 0 ? r + b : r; };

			//格式化左边界（向下对齐）
			int left_rem = mod(target_range.left - (root.X + 0.5f), block_size);
			target_range.left -= left_rem;

			//格式化右边界（向上对齐）
			int right_rem = mod(target_range.right - (root.X - 0.5f), block_size);
			target_range.right += (right_rem == 0 ? 0 : block_size - right_rem);

			//格式化上边界（向上对齐）
			int up_rem = mod(target_range.up - (root.Y - 0.5f), block_size);
			target_range.up += (up_rem == 0 ? 0 : block_size - up_rem);

			//格式化下边界（向下对齐）
			int down_rem = mod(target_range.down - (root.Y + 0.5f), block_size);
			target_range.down -= down_rem;

		}

		//待查询范围比较
		int target_range_compare(const coord_range& target_range, const coord_range& node_range)
		{
			//简化表示路径
			auto& t_left = target_range.left;
			auto& t_right = target_range.right;
			auto& t_up = target_range.up;
			auto& t_down = target_range.down;
			//简化表示路径
			auto& n_left = node_range.left;
			auto& n_right = node_range.right;
			auto& n_up = node_range.up;
			auto& n_down = node_range.down;

			//全包含判断：目标边界均在节点边界内部或重合
			if (t_left >= n_left && t_right <= n_right &&
				t_down >= n_down && t_up <= n_up)
				return ALL_IN;
			//无交集判断：目标整体在节点某一侧（左、右、下、上）
			else if (t_right < n_left || t_left > n_right ||
				t_up < n_down || t_down > n_up)
				return NONE_IN;
			//剩余情况即为部分相交
			else
				return PART_IN;
		}

		//可查询范围计算
		coord_double seekable_range_calcu(coord_range& receiver, const coord_range& target_range)
		{
			//计算四叉树管理范围
			coord_range tree_range;
			manage_range_calcu(tree_range);

			//是否申请扩大标记
			coord_double coord_sign = state.root;

			//计算可查询范围
			//左边界比较
			if (target_range.left < tree_range.left)
			{
				receiver.left = tree_range.left;
				//重置标记
				coord_sign.X = receiver.left;
			}
			else
				receiver.left = target_range.left;
			//右边界比较
			if (target_range.right > tree_range.right)
			{
				receiver.right = tree_range.right;
				//重置标记
				coord_sign.X = receiver.right;
			}
			else
				receiver.right = target_range.right;
			//上边界比较
			if (target_range.up > tree_range.up)
			{
				receiver.up = tree_range.up;
				//重置标记
				coord_sign.Y = receiver.up;
			}
			else
				receiver.up = target_range.up;
			//下边界比较
			if (target_range.down < tree_range.down)
			{
				receiver.down = tree_range.down;
				//重置标记
				coord_sign.Y = receiver.down;
			}
			else
				receiver.down = target_range.down;

			//返回是否申请扩大标记
			return coord_sign;
		}

		//子节点递归
		bool child_node_recur(Node*& this_node, const int& direct, const Node_process_mode& mode)
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
		bool tree_expand(void)
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

		//单点查询可行性分析
		int check_seekable(const coord_int& target)
		{
			/*函数逻辑：
					  0，代表分析已经结束，查找不可行
					  1，代表分析正在支持，查找可能可行
					  2，代表分析已经结束，查找可行
			*/

			//简化表示路径
			auto& root = state.root;
			//四叉树管理范围存储
			coord_range tree_range{};
			//计算四叉树管理范围
			manage_range_calcu(tree_range);

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
					if (tree_expand() == true)
					{
						//重新计算四叉树管理范围
						manage_range_calcu(tree_range);
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
	};
}

#endif
