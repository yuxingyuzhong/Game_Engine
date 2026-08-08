#pragma once
#include "配置检查器.h"

// ---------- 基础类型 ----------
using std::string;
using std::u8string;

// ---------- 错误与异常 ----------
using std::error_code;

// ---------- 文件系统 ----------
using std::filesystem::exists;
using std::filesystem::is_regular_file;
using std::filesystem::path;

// ---------- nlohmann json ----------
using nlohmann::json;
