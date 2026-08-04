#pragma once
#include "技能参数打包组件.h"

// ---------- 标准库 ----------
// 基础类型
using std::string;

// 容器
using std::unordered_map;

// 智能指针与内存分配
using std::make_unique;
using std::nothrow;
using std::unique_ptr;

// ---------- sol2 ----------
using sol::as_table;   // 将 C++ 容器包装为 Lua 表
