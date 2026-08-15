#pragma once
#include "配置加载器.h"

// ---------- 输入输出与流 ----------
using std::cout;
using std::endl;
using std::ifstream;

// ---------- 错误与异常 ----------
using std::error_code;
using std::exception;

// ---------- 函数对象与内存管理 ----------
using std::function;
using std::nothrow;      // 与 new 搭配，归于内存分配
using std::shared_ptr;

// ---------- 字符串与容器 ----------
using std::string;
using std::u8string;
using std::vector;

// ---------- 文件系统（C++20） ----------
using std::filesystem::exists;
using std::filesystem::is_regular_file;
using std::filesystem::path;
using std::filesystem::recursive_directory_iterator;

// ---------- nlohmann::json --------
using nlohmann::json;