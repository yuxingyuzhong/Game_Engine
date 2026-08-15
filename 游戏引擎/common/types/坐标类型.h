#pragma once
//预编译头
#include "common/前置头文件包含.h"

//通用模块
namespace engine
{
	//前向声明（必须放在使用它的结构体之前）
	struct coord2D_double;

	//通用坐标结构体——整数坐标
	struct coord2D_int
	{
		//构造函数
		coord2D_int() = default;
		//构造函数
		coord2D_int(const int& coord_X, const int& coord_Y)
		{
			X = coord_X;
			Y = coord_Y;
		}
		//类型转换函数
		coord2D_int(const coord2D_double& coord);
		int X = 0;
		int Y = 0;

		//等于运算符重载
		bool operator==(const coord2D_int& object) const
		{
			if (this->X == object.X && this->Y == object.Y)
				return true;
			else
				return false;
		}
		//不等于运算符重载
		bool operator!=(const coord2D_int& object) const
		{
			return !(*this == object);
		}

		// 输出重载（友元版本，放在结构体内部声明）
		friend std::ostream& operator<<(std::ostream& os, const coord2D_int& c) {
			os << std::format("X轴坐标: {} \nY轴坐标: {}\n", c.X, c.Y);
			return os;
		}
	};

	//通用坐标结构体——双精度浮点坐标
	struct coord2D_double
	{
		//构造函数
		coord2D_double() = default;
		//构造函数
		coord2D_double(const double& coord_X, const double& coord_Y)
		{
			X = coord_X;
			Y = coord_Y;
		}
		//坐标类型转换函数
		coord2D_double(const coord2D_int& coord)
		{
			X = coord.X;
			Y = coord.Y;
		}
		double X = 0.0;
		double Y = 0.0;

		//等于运算符重载
		//-----精确相等比较（基于 ULP）-----
		//使用 ULP（Unit in the Last Place）比较，可适应任意大小坐标，
		//避免绝对容差在大数值下失效的问题。
		bool operator==(const coord2D_double& other) const
		{
			// 1. 处理特殊值：NaN 和无穷
			if (std::isnan(X) || std::isnan(other.X) ||
				std::isnan(Y) || std::isnan(other.Y))
				return false;
			if (std::isinf(X) || std::isinf(other.X) ||
				std::isinf(Y) || std::isinf(other.Y))
				return X == other.X && Y == other.Y;

			// 2. 计算两个 double 之间的 ULP 距离（绝对值）
			auto ulp_distance = [](double lhs, double rhs) -> uint64_t {
				if (lhs == rhs) return 0;
				// 将 double 按位转换为 64 位整数（IEEE 754 格式）
				uint64_t lhs_bits = *reinterpret_cast<const uint64_t*>(&lhs);
				uint64_t rhs_bits = *reinterpret_cast<const uint64_t*>(&rhs);
				// 如果符号不同，则差值极大（视为不相等）
				if ((lhs_bits & 0x8000000000000000ULL) !=
					(rhs_bits & 0x8000000000000000ULL))
					return UINT64_MAX;
				// 计算整数差值（即 ULP 数量）
				if (lhs_bits > rhs_bits) std::swap(lhs_bits, rhs_bits);
				return rhs_bits - lhs_bits;
				};

			const uint64_t max_ulp = 4;  // 允许 4 个 ULP 误差（通常足够）
			return ulp_distance(X, other.X) <= max_ulp &&
				ulp_distance(Y, other.Y) <= max_ulp;
		}
		//不等于运算符重载
		bool operator!=(const coord2D_double& other) const
		{
			return !(*this == other);
		}
		//赋值运算符重载
		coord2D_double& operator=(const coord2D_double& other)
		{
			X = other.X;
			Y = other.Y;
			return *this;
		}

		// 输出重载（友元版本，放在结构体内部声明）
		friend std::ostream& operator<<(std::ostream& os, const coord2D_double& c) {
			os << std::format("X轴坐标: {} \nY轴坐标: {}\n", c.X, c.Y);
			return os;
		}
	};

	//类型转换函数实现
	inline coord2D_int::coord2D_int(const coord2D_double& coord)
	{
		X = static_cast<int>(std::lround(coord.X));
		Y = static_cast<int>(std::lround(coord.Y));
	}

	//范围坐标结构体
	struct coord2D_range
	{
		int left = 0;
		int right = 0;
		int up = 0;
		int down = 0;

		//等于运算符重载
		bool operator==(const coord2D_range& other) const
		{
			return left == other.left && right == other.right &&
				up == other.up && down == other.down;
		}
		//不等于运算符重载
		bool operator!=(const coord2D_range& other) const
		{
			return !(*this == other);
		}

		// 输出重载（友元版本，放在结构体内部声明）
		friend std::ostream& operator<<(std::ostream& os, const coord2D_range& c) {
			os << std::format("左边界: {} \n右边界: {}\n上边界: {} \n下边界: {}\n"
				, c.left, c.right, c.up, c.down);
			return os;
		}
	};
}
