#pragma once
#include "前置头文件包含.h"

// —— C++ 标准库 I/O 与格式化 ——
using std::cin;
using std::cout;
using std::endl;
using std::format;
using std::iostream;
using std::ifstream;
using std::ofstream;
using std::istreambuf_iterator;

// —— C++ 字符串与正则 ——
using std::string;

// —— C++ 容器与迭代器 ——
using std::vector;
using std::array;
using std::unordered_set;
using std::unordered_map;
using std::unique_ptr;

// —— C++ 通用工具 ——
using std::nothrow;
using std::source_location;
using std::function;
using std::copy;
using std::move;
using std::swap;
using std::rotate;
using std::popcount;

// —— C++ 数学 ——
using std::sqrt;
using std::abs;
using std::lround;
using std::max;

// —— C++20 ranges 算法与比较器 ——
using std::ranges::greater;
using std::ranges::less;
using std::ranges::sort;
using std::ranges::fill;

//前向声明名称空间

//通用模块
namespace Game_Engine
{
	//前向声明（必须放在使用它的结构体之前）
	struct coord_double;

	//通用坐标结构体——整数坐标
	struct coord_int
	{
		//构造函数
		coord_int() = default;
		//构造函数
		coord_int(const int& coord_X, const int& coord_Y)
		{
			X = coord_X;
			Y = coord_Y;
		}
		//类型转换函数
		coord_int(const coord_double& coord);
		int X = 0;
		int Y = 0;

		//等于运算符重载
		bool operator==(const coord_int& object) const
		{
			if (this->X == object.X && this->Y == object.Y)
				return true;
			else
				return false;
		}
		//不等于运算符重载
		bool operator!=(const coord_int& object) const
		{
			return !(*this == object);
		}

		// 输出重载（友元版本，放在结构体内部声明）
		friend std::ostream& operator<<(std::ostream& os, const coord_int& c) {
			os << format("X轴坐标: {} \nY轴坐标: {}\n", c.X, c.Y);
			return os;
		}
	};

	//通用坐标结构体——双精度浮点坐标
	struct coord_double
	{
		//构造函数
		coord_double() = default;
		//构造函数
		coord_double(const double& coord_X, const double& coord_Y)
		{
			X = coord_X;
			Y = coord_Y;
		}
		//坐标类型转换函数
		coord_double(const coord_int& coord)
		{
			X = coord.X;
			Y = coord.Y;
		}
		double X = 0.0;
		double Y = 0.0;

		//等于运算符重载
		bool operator==(const coord_double& other) const
		{
			const double eps = 1e-9;  // 容差范围，适配double精度
			return std::abs(X - other.X) < eps && std::abs(Y - other.Y) < eps;
		}
		//不等于运算符重载
		bool operator!=(const coord_double& other) const
		{
			return !(*this == other);
		}

		// 输出重载（友元版本，放在结构体内部声明）
		friend std::ostream& operator<<(std::ostream& os, const coord_double& c) {
			os << format("X轴坐标: {} \nY轴坐标: {}\n", c.X, c.Y);
			return os;
		}
	};

	//类型转换函数实现
	inline coord_int::coord_int(const coord_double& coord)
	{
		X = static_cast<int>(std::lround(coord.X));
		Y = static_cast<int>(std::lround(coord.Y));
	}

	//范围坐标结构体
	struct coord_range
	{
		int left = 0;
		int right = 0;
		int up = 0;
		int down = 0;

		//等于运算符重载
		bool operator==(const coord_range& other) const
		{
			return left == other.left && right == other.right &&
				up == other.up && down == other.down;
		}
		//不等于运算符重载
		bool operator!=(const coord_range& other) const
		{
			return !(*this == other);
		}

		// 输出重载（友元版本，放在结构体内部声明）
		friend std::ostream& operator<<(std::ostream& os, const coord_range& c) {
			os << format("左边界: {} \n右边界: {}\n上边界: {} \n下边界: {}\n"
				, c.left, c.right, c.up, c.down);
			return os;
		}
	};
}

//四叉树及四叉树管理器模块
namespace Game_Engine
{
	//类模板前向声明
	template<typename T> class Quadtree;
	//类模板前向声明
	template<typename T> class Quadtree_Manager;

	//四叉树节点结构体
	//用于四叉树管理器
	template<typename T>
	struct tree_record
	{
		//四叉树指针
		Quadtree<T>* tree = nullptr;
		//四叉树根节点坐标
		coord_double root{ 0.5,0.5 };
		//四叉树大小
		uint16_t size = 256;
		//四叉树编号
		uint16_t quadtree_ID = 0;
	};

	//四叉树合并/卸载信息结构体
	//用于四叉树管理器，角色管理器
	template<typename T>
	struct merge_feedback
	{
	private:
		vector<tree_record<T>*> ptr_tree{};
	public:
		//待合并/卸载四叉树ID
		vector<uint16_t> old_tree_ID{};
		//新建四叉树ID
		vector<uint16_t> new_tree_ID{};
		//四叉树管理器友元
		friend class Quadtree_Manager<T>;
	};

	// 四叉树输出信息结构体
	// 用于四叉树及四叉树管理器，食物类
	template<typename T>
	struct tree_block_data
	{
	public:
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

	//四叉树管理器输出信息结构体
	//用于四叉树管理器，事件中心，角色管理器
	template<typename T>
	struct tree_manager_handle
	{
	private:
		//四叉树及其信息指针
		tree_record<T>* ptr_tree = nullptr;
	public:
		//默认构造函数
		tree_manager_handle()
		{}
		tree_manager_handle(tree_record<T>* tree, int id, tree_block_data<T>* data_ptr)
			: ptr_tree(tree), ID(id), ptr_data(data_ptr)
		{}
		//默认析构函数
		~tree_manager_handle()
		{

		}
		//四叉树管理器友元
		friend class Quadtree_Manager<T>;
		//四叉树编号
		//树ID从1开始
		//0为初始标记
		uint16_t ID = 0;
		//查找目标区块
		tree_block_data<T>* ptr_data = nullptr;
	};

}

//命名空间别名
namespace engine = Game_Engine;

//全局使用通用坐标表示
using engine::coord_int;
using engine::coord_double;
using engine::coord_range;

