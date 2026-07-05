#ifndef THIS_ALGORITHM
#define THIS_ALGORITHM
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"

//通用算法模块
namespace Game_Engine
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
		return -1;
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
//使用二分查找通用算法
using engine::binary_search;
//使用二项式值计算算法
using engine::bino_distr_calcu;


#endif 