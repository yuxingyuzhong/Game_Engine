#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"

//通用算法模块
namespace Game_Engine
{
	// 修复后的 point_binary_search：返回全局索引（相对于原始 first）
	template<typename RandomIt, typename T, typename Compare, typename Projection = std::identity>
	int point_binary_search(RandomIt first, RandomIt last, const T& target,
		const Compare& comp, Projection proj = {})
	{
		RandomIt original_first = first;  // 保存原始起始位置
		while (first < last) {
			RandomIt mid = first + (last - first) / 2;
			if (comp(target, std::invoke(proj, *mid)))
				last = mid;
			else if (comp(std::invoke(proj, *mid), target))
				first = mid + 1;
			else
				return static_cast<int>(mid - original_first);  // 使用原始起始位置计算索引
		}
		//若查找失败则返回无效索引
		return -1;
	}

	template<typename RandomIt, typename T, typename Compare,
		typename Projection = std::identity>
	std::pair<int, int> range_binary_search(RandomIt first, RandomIt last,
		const T& target,
		const Compare& comp,
		Projection proj = {})
	{
		// 1. 计算下界：第一个使得 comp(proj(*it), target) 为 false 的元素
		//    即第一个不小于 target 的元素
		auto lower = first;
		auto end = last;
		auto count = std::distance(first, last);
		while (count > 0) {
			auto step = count / 2;
			auto it = lower;
			std::advance(it, step);
			if (comp(proj(*it), target)) {
				lower = ++it;
				count -= step + 1;
			}
			else {
				count = step;
			}
		}

		// 2. 计算上界：第一个使得 comp(target, proj(*it)) 为 true 的元素
		//    即第一个大于 target 的元素
		auto upper = lower;      // 从上界开始搜索
		count = std::distance(upper, last);
		while (count > 0) {
			auto step = count / 2;
			auto it = upper;
			std::advance(it, step);
			if (!comp(target, proj(*it))) {   // 即 target >= proj(*it) 
				upper = ++it;
				count -= step + 1;
			}
			else {
				count = step;
			}
		}

		// 3. 检查下界是否有效且与目标等价
		if (lower == last || comp(proj(*lower), target) || comp(target, proj(*lower))) {
			return { -1, -1 };   // 未找到任何等价元素
		}

		// 4. 返回闭区间的下标
		auto left_index = static_cast<int>(std::distance(first, lower));
		auto right_index = static_cast<int>(std::distance(first, upper)) - 1;
		return { left_index, right_index };
	}

	//特定位数 数值和计算（引用版本）
	template <typename T>
	void bit_calcu(int& receiver,const T& bits, const int& begin,int end = -1)
	{
		//若未输入结束位
		//则继承开始位
		if (end == -1)
			end = begin;

		receiver = 0;
		int weight = 1;
		for (int i = begin; i <= end; ++i)
		{
			if ((bits >> i) & 1)
				receiver += weight;
			weight <<= 1;
		}
	}

	//特定位数 数值和计算（返回值版本）
	template <typename T>
	int bit_calcu(const T& bits, const int& begin,int end = -1)
	{
		//若未输入结束位
		//则继承开始位
		if (end == -1)
			end = begin;
		int output = 0;
		int weight = 1;
		for (int i = begin; i <= end; ++i)
		{
			if ((bits >> i) & 1)
				output += weight;
			weight <<= 1;
		}
		return output;
	}

	//特定位数 数值和设置（保持不变，已是正序）
	template <typename T>
	void bit_set(T& bits, const int& begin, const int& end, const int& size)
	{
		// 清除区间内所有位
		for (int i = begin; i <= end; ++i)
			bits &= ~(1U << i);
		// 设置新的值
		int weight = 1;
		for (int i = begin; i <= end; ++i)
		{
			if (size & weight)
				bits |= (1U << i);
			weight <<= 1;
		}
	}

	//二项分布值计算（递推版本）
	inline void bino_distr_calcu(const int& test_time, const int& success_time,
		const double& probability, double& receiver)
	{
		// 边界处理：p = 0 或 p = 1
		if (probability == 0.0) {
			receiver = (success_time == 0) ? 1.0 : 0.0;
			return;
		}
		if (probability == 1.0) {
			receiver = (success_time == test_time) ? 1.0 : 0.0;
			return;
		}

		// 递推初始值 P(0) = (1-p)^n
		double pk = pow(1.0 - probability, test_time);
		if (success_time == 0) {
			receiver = pk;
			return;
		}

		// 递推比率
		double ratio = probability / (1.0 - probability);

		// 递推到目标 k
		for (int k = 1; k <= success_time; ++k) {
			pk = pk * (test_time - k + 1) / k * ratio;
		}
		receiver = pk;
	}
}

//使用位操作通用算法
using engine::bit_set;
using engine::bit_calcu;
//使用点二分查找算法
using engine::point_binary_search;
//使用范围二分查找算法
using engine::range_binary_search;
//使用二项式值计算算法
using engine::bino_distr_calcu;

