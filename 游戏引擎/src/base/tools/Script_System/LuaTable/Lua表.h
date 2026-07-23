#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"

namespace Game_Engine
{
	//Lua表
	class LuaTable
	{
	public:
		// 新增：默认构造一个无效的 LuaTable
		LuaTable() : L_(nullptr), ref_(-1) {}

		// 默认构造：创建一个空的 LuaTable（内部指向一个空表）
		LuaTable(lua_State* L);

		// 从栈上的指定位置构造：将那个 table 存入注册表
		LuaTable(lua_State* L, int index);

		// 析构：从注册表中释放引用
		~LuaTable();

		// 禁止拷贝，允许移动
		LuaTable(const LuaTable&) = delete;
		LuaTable& operator=(const LuaTable&) = delete;
		LuaTable(LuaTable&& other) noexcept;
		LuaTable& operator=(LuaTable&& other) noexcept;

		//读取布尔值
		bool get_bool(const string& key, bool default_value = false);

		//读取数值
		double get_number(const string& key, double default_value = 0.0) const;

		//读取字符串
		std::string get_string(const string& key, const string& default_value = "") const;

		//检查是否有效
		bool is_valid() const;

	private:
		void release(); // 辅助：释放引用

		lua_State* L_;
		int ref_;      // 注册表中的引用索引，-1 表示无效
	};

}
