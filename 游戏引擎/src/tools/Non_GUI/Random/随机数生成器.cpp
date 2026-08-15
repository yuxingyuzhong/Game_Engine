#include "随机数生成器.h"

//使用随机数生成器
using engine::Random_Generator;

// ======== 原始 PCG32 核心（返回 uint32_t）========
uint32_t Random_Generator::pcg32_random_raw()
{
    uint64_t old_state = g_pcg32.state;
    g_pcg32.state = old_state * 6364136223846793005ULL + g_pcg32.inc;
    uint32_t xorshifted = (uint32_t)(((old_state >> 18u) ^ old_state) >> 27u);
    uint32_t rot = (uint32_t)(old_state >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-(int)rot) & 31));
}

// ======== 种子初始化（重载）========
void Random_Generator::pcg32_seed_init(uint64_t seed_out)
{
    g_pcg32.state = 0;
    g_pcg32.inc = (seed_out << 1u) | 1u;
    (void)pcg32_random_raw();   // 预热一次
}

void Random_Generator::pcg32_seed_init()
{
    static uint64_t counter = 0;
    auto now = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    uint64_t seed = static_cast<uint64_t>(ns) ^ static_cast<uint64_t>(std::time(nullptr)) ^ (++counter);
    g_pcg32.state = 0;
    g_pcg32.inc = (seed << 1u) | 1u;
    (void)pcg32_random_raw();
}

// ======== 构造函数 ========
Random_Generator::Random_Generator()
{
    pcg32_seed_init();
}

Random_Generator::Random_Generator(uint64_t seed_out)
{
    pcg32_seed_init(seed_out);
}

// ======== 生成 64 位随机数（组合两次 32 位）========
int64_t Random_Generator::operator()()
{
    uint64_t high = pcg32_random_raw();
    uint64_t low = pcg32_random_raw();
    return static_cast<int64_t>((high << 32) | low);
}

// ======== 生成 [min, max] 范围内的 64 位随机数（无偏）========
int64_t Random_Generator::pcg64_random_range(int64_t min, int64_t max)
{
    if (min > max) {
        int64_t tmp = min;
        min = max;
        max = tmp;
    }

    uint64_t range = static_cast<uint64_t>(max - min) + 1ULL;
    if (range == 0) {  // 处理整个 int64_t 范围的情况
        // 直接生成完整 64 位随机数并返回（概率极低，但无偏）
        return operator()();
    }
    if (range == 1) return min;

    // 拒绝采样法（范围可能接近 2^64，需要用 128 位辅助）
    uint64_t limit = UINT64_MAX - UINT64_MAX % range;
    uint64_t r;
    do {
        r = static_cast<uint64_t>(operator()());
    } while (r >= limit);
    return min + static_cast<int64_t>(r % range);
}

// 公有接口
int64_t Random_Generator::operator()(int64_t min, int64_t max)
{
    return pcg64_random_range(min, max);
}