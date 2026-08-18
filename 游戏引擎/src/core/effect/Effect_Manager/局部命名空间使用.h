#pragma once
#include "效应管理器.h"

// ---------- 基础类型 ----------
using std::string;
using std::hash;

// ---------- 容器 ----------
using std::unordered_map;
using std::vector;

// ---------- 智能指针 ----------
using std::shared_ptr;
using std::unique_ptr;

// ---------- 可选值 ----------
using std::nullopt;
using std::optional;

// ---------- 算法与比较 ----------
using std::move;
using std::swap;
using std::ranges::greater;
using std::ranges::less;
using std::ranges::sort;
using std::ranges::find;

// ----------- 数学 --------------
using std::numeric_limits;

// ---------- 内存分配 ------------
using std::nothrow;

// ---------- nlohmann json ----------
using nlohmann::json;  
