#pragma once
//============================================================================
// 前置头文件包含（预编译头）
// 包含所有常用 C/C++ 标准库头文件及平台相关头文件
//============================================================================

//============================================================================
// 1. C 标准库（C89/C99）
//============================================================================
#include <stdio.h>      // printf, scanf, fopen, fclose
#include <ctype.h>      // isalpha, isdigit, tolower, toupper
#include <string.h>     // strlen, strcpy, strcmp, strcat
#include <stdlib.h>     // malloc, free, atoi, rand, srand, exit, qsort
#include <stdint.h>     // int8_t, uint16_t, int32_t, uint64_t
#include <time.h>       // time, localtime, strftime, clock
#include <cassert>      // assert（C++ 风格）

//============================================================================
// 2. C++ 标准库（按功能分组）
//============================================================================

// ----- 通用实用工具 -----
#include <utility>      // std::pair, std::move, std::swap
#include <tuple>        // std::tuple
#include <optional>     // std::optional (C++17)
#include <variant>      // std::variant (C++17)
#include <any>          // std::any (C++17)
#include <memory>       // std::unique_ptr, std::shared_ptr, std::weak_ptr
#include <type_traits>  // 类型特性（is_same, enable_if, etc.）
#include <functional>   // std::function, std::bind, std::greater, std::less

// ----- 容器 -----
#include <vector>       // std::vector
#include <list>         // std::list
#include <deque>        // std::deque
#include <set>          // std::set, std::multiset
#include <map>          // std::map, std::multimap
#include <unordered_set>// std::unordered_set, std::unordered_multiset
#include <unordered_map>// std::unordered_map, std::unordered_multimap

// ----- 字符串与 I/O -----
#include <string>       // std::string
#include <iostream>     // std::cin, std::cout, std::cerr, std::endl
#include <fstream>      // std::ifstream, std::ofstream, std::fstream
#include <sstream>      // std::stringstream
#include <format>       // std::format (C++20)
#include <filesystem>   

// ----- 算法与迭代器 -----
#include <algorithm>    // sort, find, reverse, swap, max_element, min_element
#include <iterator>     // std::distance, std::advance, std::back_inserter
#include <numeric>      // std::accumulate, std::iota, std::reduce

// ----- 数学与位操作 -----
#include <cmath>        // std::sin, std::cos, std::sqrt, std::pow, std::abs
#include <numbers>      // std::numbers::pi, etc. (C++20)
#include <bit>          // std::popcount, std::rotl, std::bit_width (C++20)
#include <limits>

// ----- 时间与随机数 -----
#include <chrono>       // std::chrono::high_resolution_clock, durations
#include <random>       // std::mt19937, std::uniform_real_distribution

// ----- 多线程与并发 (C++11) -----
#include <thread>       // std::thread, std::this_thread
#include <mutex>        // std::mutex, std::lock_guard, std::unique_lock
#include <atomic>       // std::atomic<T>
#include <future>       // std::future, std::promise, std::async
#include <barrier>      // std::barrier (C++20)

// ----- 范围与视图 (C++20) -----
#include <ranges>       // std::ranges::sort, std::views::filter, etc.

// ----- C++20 调试信息 -----
#include <source_location>  // std::source_location

// ----- Lua C++ API（外部库）-----
extern "C"
{
	#include <lua.h>       // Lua 核心 API
    #include <lauxlib.h>   // Lua 辅助库（luaL_newstate 等）
    #include <lualib.h>    // Lua 标准库（luaL_openlibs 等）
}

// ----- JSON 解析（外部库）-----
#include <nlohmann/json.hpp>

// ----- Lua 封装（外部库）-----
#include <sol/sol.hpp>  // Sol2 的唯一入口

// ----- OpenGL 加载器（GLAD）-----
#include <glad/gl.h>

// ----- 窗口与输入（GLFW）-----
#include <GLFW/glfw3.h>

// ----- Dear ImGui UI 库 -----
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

//============================================================================
// 3. 编译器与平台相关（仅 Windows）
//============================================================================
#ifdef _WIN32
#include <windows.h>    // Windows API
#endif