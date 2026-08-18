#pragma once
//预编译头
#include "common/前置头文件包含.h"

//通用算法模块
namespace engine
{
	// 修复后的 binary_search：返回全局索引（相对于原始 first）
	template<typename RandomIt, typename T, typename Compare, typename Projection = std::identity>
	int binary_search(RandomIt first, RandomIt last, const T& target,
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

	// ----- 新增容器重载（直接传递容器） -----
    // binary_search 的容器版本
	template<typename Container, typename T, typename Compare, typename Projection = std::identity>
	int binary_search(const Container& container, const T& target,
		const Compare& comp, Projection proj = {})
	{
		using std::begin;
		using std::end;
		return binary_search(begin(container), end(container),
			target, comp, proj);
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

	// ----- 新增容器重载（直接传递容器） -----
	// range_binary_search 的容器版本
	template<typename Container, typename T, typename Compare, typename Projection = std::identity>
	std::pair<int, int> range_binary_search(const Container& container, const T& target,
		const Compare& comp, Projection proj = {})
	{
		using std::begin;
		using std::end;
		return range_binary_search(begin(container), end(container),
			target, comp, proj);
	}

}

