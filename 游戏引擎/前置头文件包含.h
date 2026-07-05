#ifndef THIS_DEFINE_BEFORE
#define THIS_DEFINE_BEFORE

//============================================================================
// 1. C 标准库头文件（C89/C99）
//    提供最基础的语言扩展、输入输出、内存管理、字符串、数学、时间、断言等
//============================================================================

// 标准输入输出（printf、scanf、fopen、fclose 等核心 I/O 函数）
#include <stdio.h>

// 字符处理函数（isalpha、isdigit、tolower、toupper 等）
#include <ctype.h>

// C 风格字符串处理（strlen、strcpy、strcmp、strcat 等）
#include <string.h>

// 通用工具函数（malloc、free、atoi、rand、srand、exit、qsort 等）
#include <stdlib.h>

// 固定宽度整数类型（int8_t、uint16_t、int32_t、uint64_t 等，C99 标准）
#include <stdint.h>

// C 风格时间/日期处理（time、localtime、strftime、clock 等）
#include <time.h>

//============================================================================
// 2. C++ 标准库头文件（C++98/11/14/17/20）
//    按功能分组：通用算法、容器、I/O、字符串、函数对象、线程支持等
//============================================================================

// ----- 通用算法与工具 -----
// STL 算法库（sort、find、reverse、swap、max_element、min_element 等）
#include <algorithm>

// ----- C++20 源代码位置信息 -----
// 提供 std::source_location 类，用于获取文件名、行号、函数名等调试信息
#include <source_location>

// 无序关联容器（std::unordered_map，基于哈希表的映射容器）
#include <unordered_map>

// 无序集合容器（std::unordered_set，基于哈希表的集合容器）
#include <unordered_set>

// C++ 字符串类（std::string，支持动态扩容、编码转换、查找替换等）
#include <string>

// ----- 输入输出与格式化 -----
// C++ 输入输出流（std::cin、std::cout、std::cerr、std::endl 等）
#include <iostream>

// C++ 文件流（std::ifstream、std::ofstream、std::fstream）
#include <fstream>

// C++20 格式化输出（std::format、std::format_to 等）
#include <format>

// ----- 数学函数库 -----
// C++ 数学函数库（提供 std::sin、std::cos、std::sqrt、std::pow、std::abs 等）
#include <cmath>

// ----- 函数对象与时间 -----
// 通用函数包装器（std::function，可存储 lambda、函数指针、可调用对象）
#include <functional>

// ----- 位操作与类型特性（C++20）-----
// 位操作函数（std::popcount、std::rotl、std::rotr、std::bit_width 等）
#include <bit>

// ----- 集合容器 -----
// 有序集合容器（std::set，基于红黑树，元素唯一且自动排序）
#include <set>

// ----- C++20 Ranges 库 -----
// 提供 std::ranges::sort、std::ranges::greater 等范围算法与比较器
#include <ranges>

// ----- 多线程与并发（C++11 起）-----
// 线程支持库：std::thread、std::this_thread 命名空间
#include <thread>
// 互斥锁与同步原语：std::mutex、std::lock_guard、std::unique_lock、std::condition_variable
#include <mutex>
// 原子操作：std::atomic<T>，用于无锁编程
#include <atomic>
// 异步任务与 future：std::future、std::promise、std::async
#include <future>
// C++20 屏障，用于线程同步
#include <barrier>   

// 包含windows头文件用于调用API
#include <windows.h>

// ----- 编译器内置函数（仅 MSVC）-----
// 提供 __cpuid、__rdtsc、__rdtscp 等硬件相关底层函数
#include <intrin.h>


#endif // THIS_DEFINE_BEFORE