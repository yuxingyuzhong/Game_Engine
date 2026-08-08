#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "src/base/tools/Random/随机数生成器.h"
#include "src/base/space/Quadtree/四叉树.h"
#include "src/base/space/Quadtree_Manager/四叉树管理器.h"

//使用随机数生成器
using engine::Random_Generator;
//使用四叉树输出结构体
using engine::tree_chunk_data;
//使用四叉树
using engine::Quadtree;
//使用四叉树生成器
using engine::Quadtree_Manager;

namespace engine
{
	//坐标生成
	class Gen_Coord
	{
	private:
		//随机数生成引擎
		Random_Generator gen_num;
	public:
		Gen_Coord(void)
		{

		}
		~Gen_Coord(void)
		{

		}
		//一键生成整数
		int64_t operator ()(void)
		{
			return gen_num();
		}
		//一键生成整数
		int64_t operator ()(int64_t min, int64_t max)
		{
			return gen_num(min, max);
		}
		//一键生成整数坐标
		coord2D_int gen_coord(const int64_t& min, const int64_t& max)
		{
			//坐标临时存储
			coord2D_int coord{};
			//生成X轴坐标
			coord.X = gen_num(min, max);
			//生成Y轴坐标
			coord.Y = gen_num(min, max);
			//返回生成结果
			return coord;
		}
		//一键生成坐标范围
		coord2D_range gen_range(const int64_t& min, const int64_t& max)
		{
			//坐标范围临时存储
			coord2D_range range{};
			//循环生成坐标范围
			for (; ;)
			{
				//生成左右边界
				if (range.left >= range.right)
				{
					range.left = gen_num(min, max);
					range.right = gen_num(min, max);
				}
				//生成上下边界
				if (range.down >= range.up)
				{
					range.down = gen_num(min, max);
					range.up = gen_num(min, max);
				}

				//若生成范围正常则返回
				if (range.left < range.right && range.down < range.up)
					return range;
			}
		}
		//坐标格式化
		void format_coord(coord2D_double& coord, const coord2D_double& root, const uint64_t block_size)
		{
			// 匹配坐标临时存储（逐步调整至最终区块中心）
			coord2D_double catch_coord = root;

			// 区块范围临时存储（整数边界）
			coord2D_range range{};

			// ---------- 初始化区块范围（修正：右边界和上边界用加号） ----------
			// 左边界：root.X - 0.5 - (block_size/2 - 1)
			range.left = (root.X - 0.5) - ((block_size / 2) - 1);
			// 右边界：root.X + 0.5 + (block_size/2 - 1)   ← 修正点（原为减号）
			range.right = (root.X + 0.5) + ((block_size / 2) - 1);
			// 下边界：root.Y - 0.5 - (block_size/2 - 1)
			range.down = (root.Y - 0.5) - ((block_size / 2) - 1);
			// 上边界：root.Y + 0.5 + (block_size/2 - 1)   ← 修正点（原为减号）
			range.up = (root.Y + 0.5) + ((block_size / 2) - 1);

			// ---------- 循环平移区块范围，直到完全包裹目标坐标 ----------
			for (;;)
			{
				// 若目标坐标在左边界外，整体左移一个区块
				if (coord.X < range.left)
				{
					range.left -= block_size;
					range.right -= block_size;
					catch_coord.X -= block_size;
				}
				// 若目标坐标在右边界外，整体右移一个区块
				else if (coord.X > range.right)
				{
					range.left += block_size;
					range.right += block_size;
					catch_coord.X += block_size;
				}

				// 若目标坐标在上边界外，整体上移一个区块（注意Y轴向上增长）
				if (coord.Y > range.up)
				{
					range.down += block_size;
					range.up += block_size;
					catch_coord.Y -= block_size;
				}
				// 若目标坐标在下边界外，整体下移一个区块
				else if (coord.Y < range.down)
				{
					range.down -= block_size;
					range.up -= block_size;
					catch_coord.Y += block_size;
				}

				// 检测目标坐标是否已完全落在当前区块范围内
				if (coord.X >= range.left && coord.X <= range.right &&
					coord.Y >= range.down && coord.Y <= range.up)
				{
					// 将最终调整得到的区块中心坐标赋给输出参数
					coord = catch_coord;
					return;
				}
			}
		}
		//范围格式化
		void format_range(coord2D_range& range, const coord2D_double& root, const uint64_t block_size)
		{

			//格式化目的:保证待查询范围严格包含内部区块

			// 安全取模（处理负数）
			auto mod = [](int a, int b) { int r = a % b; return r < 0 ? r + b : r; };

			//格式化左边界（向下对齐）
			int left_rem = mod(range.left - (root.X + 0.5f), block_size);
			range.left -= left_rem;

			//格式化右边界（向上对齐）
			int right_rem = mod(range.right - (root.X - 0.5f), block_size);
			range.right += (right_rem == 0 ? 0 : block_size - right_rem);

			//格式化上边界（向上对齐）
			int up_rem = mod(range.up - (root.Y - 0.5f), block_size);
			range.up += (up_rem == 0 ? 0 : block_size - up_rem);

			//格式化下边界（向下对齐）
			int down_rem = mod(range.down - (root.Y + 0.5f), block_size);
			range.down -= down_rem;
		}
	};

	//测试专用数据结构
	class Test_Data
	{
		//值存储
		int64_t store = 0;
	public:
		//等于运算符重载
		bool operator ==(const Test_Data& other)
		{
			//若存储值相等则相等
			if (this->store == other.store)
				return true;
			else
				return false;
		}
		//静态比较函数
		static bool compare(const tree_chunk_data<Test_Data>& itself,
			const tree_chunk_data<Test_Data>& other)
		{
			//输出具体比较信息
			cout << format("预期目标值: {} / 实际目标值: {}\n",
				itself.ptr_data->store, other.ptr_data->store);
			cout << format("预期坐标值: \n");
			cout << itself.node;
			cout << format("实际坐标值: \n");
			cout << other.node;
			//若所有数据均匹配则返回true
			if (itself.node == other.node && itself.ptr_data == other.ptr_data)
			{
				cout << format("匹配成功!\n");
				return true;
			}
			//若不完全匹配则进行处理
			else
			{
				cout << format("匹配发生错误!\n");
				return false;
			}
		}
		//数值设置
		static void set_num(const tree_chunk_data<Test_Data>& itself,
			const int64_t& num)
		{
			itself.ptr_data->store = num;
		}
		//数据复制
		static void copy(tree_chunk_data<Test_Data>& receiver,tree_chunk_data<Test_Data>& transmiter)
		{
			//链接数据
			receiver.ptr_data = transmiter.ptr_data;
			//置空指针
			transmiter.ptr_data = nullptr;
			//继承坐标
			receiver.node = transmiter.node;
		}
		//坐标输出
		static void out(const tree_chunk_data<Test_Data>& itself)
		{
			//输出坐标
			cout << itself.node;
		}
	};

	class Tester
	{
	private:
		Gen_Coord gen;
	public:
		//四叉树单点查找测试
		void qurdtree_block_seek_test(const uint64_t& tree_size_start,
			const int64_t& coord_radius_start, const int64_t& coord_radius_max,
			const uint64_t& block_size,const coord2D_double& root,uint64_t test_times);
		//四叉树范围查找测试
		void qurdtree_range_seek_test(const uint64_t& tree_size_start,
			const int64_t& range_radius_start, const int64_t& range_radius_max,
			const uint64_t& block_size, const coord2D_double& root, uint64_t test_times);
		//四叉树管理器单点查找测试
		void manager_block_seek_test(const uint64_t& tree_size_start, const uint64_t& tree_size_max,
			const int64_t& coord_radius_start, const int64_t& coord_radius_max,
			const uint64_t& block_size, const coord2D_double& root, uint64_t test_times);
		//四叉树管理器范围查找测试
		void manager_range_seek_test(const uint64_t& tree_size_start, const uint64_t& tree_size_max,
			const int64_t& range_radius_start, const int64_t& range_radius_max,
			const uint64_t& block_size, const coord2D_double& root, uint64_t test_times);
		//四叉树智能创建测试
		void manager_tree_build_merge_test(const uint64_t& tree_size_start, const uint64_t& tree_size_max ,
			const int64_t& coord_radius_start, const int64_t& coord_radius_max,
			const uint64_t& block_size,uint64_t test_times);
	};
}
