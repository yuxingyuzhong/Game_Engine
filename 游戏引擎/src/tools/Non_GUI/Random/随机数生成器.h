#pragma once
//预编译头
#include "common/前置头文件包含.h"

namespace engine
{
	class Random_Generator
	{
		// PCG32 状态结构体
		struct pcg32
		{
			uint64_t state = 0;
			uint64_t inc = 0;
		} g_pcg32;

		uint64_t seed = 0;

		// 底层原始 32 位随机数生成（PCG32 核心）
		uint32_t pcg32_random_raw();

		// 生成 [min, max] 范围内的 64 位随机数（无偏）
		int64_t pcg64_random_range(int64_t min, int64_t max);

	public:
		// 生成一个 int64_t 全范围随机数
		int64_t operator()();

		// 生成 [min, max] 范围内的 int64_t 随机数（包含两端）
		int64_t operator()(int64_t min, int64_t max);

		// 构造函数
		Random_Generator(void) ;
		explicit Random_Generator(uint64_t seed_out);

		// 种子初始化
		void pcg32_seed_init(uint64_t seed_out);
		void pcg32_seed_init();
	};
}
