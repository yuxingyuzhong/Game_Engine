#include "src/base/tools/Script_System/LuaScript/局部命名空间使用.h"

//构造函数：创建一个独立的 Lua 运行环境
LuaScript::LuaScript()
{
	//创建全新的 Lua 状态机，确保脚本之间相互隔离
	state_ = luaL_newstate();
	//只加载最基础的安全库（数学、字符串等），不加载 IO/os 等危险库
	luaL_openlibs(state_);
}

//析构：安全销毁 Lua 状态机
LuaScript::~LuaScript()
{
	//若存在Lua状态机则进行销毁
	if (state_)
		lua_close(state_);
}

//复制脚本
void LuaScript::copy_script(const LuaScript& script)
{
	//复制脚本
	*this = script;
}

//加载脚本文件
bool LuaScript::load_script(const string& script_path, string& error_out)
{
	//若脚本文件加载编译失败
	if (luaL_loadfile(state_, script_path.c_str()) != LUA_OK)
	{
		//加载错误信息
		error_out = lua_tostring(state_, -1);
		//弹出错误信息
		lua_pop(state_, 1);
		return false;
	}

	//生成脚本可执行函数
	if (lua_pcall(state_, 0, 0, 0) != LUA_OK)
	{
		//加载错误信息
		error_out = lua_tostring(state_, -1);
		//弹出错误信息
		lua_pop(state_, 1);
		return false;
	}

	//获取脚本类型信息
	lua_getglobal(state_, "type");
	//若类型信息存在
	if (lua_isnumber(state_, -1))
	{
		//存储类型信息
		Script_type = static_cast<Lua_type>(lua_tointeger(state_, -1));
		//将信息弹回栈
		lua_pop(state_, 1);
	}
	else
		return false;

	return true;
}

//运行数据槽装载
void LuaScript::data_slot_loading(const unordered_map<string, double>& data_slot)
{
	//装载数据
	run_data_slot = data_slot;
}

//脚本类型获取
engine::Lua_type LuaScript::script_type_get(void)
{
	return Script_type;
}

//脚本存在状态获取
bool LuaScript::is_script_alive(void)
{
	//运行函数名存储
	string function_name = "alive";
	// ================================================================
	// 第一步：在 Lua 全局表中查找目标函数
	// ================================================================
	lua_getglobal(state_, function_name.c_str());

	// 检查栈顶是不是一个函数
	if (!lua_isfunction(state_, -1))
	{
		//弹出非目标值
		lua_pop(state_, 1);
		return false;
	}

	if (lua_pcall(state_, 0, 1, 0) != LUA_OK)
	{
		//函数执行出错了，错误信息在栈顶
		string error = lua_tostring(state_, -1);
		//弹出错误信息
		lua_pop(state_, 1);
		return false;
	}

	// ================================================================
	// 第四步：处理返回值
	// ================================================================
	LuaTable result;
	if (lua_istable(state_, -1))
		result = LuaTable(state_, -1);
	lua_pop(state_, 1);
	//返回脚本存在状态
	return result.get_bool("existence", false);
}

//执行脚本函数
LuaTable LuaScript::act(const unordered_map<string, double>& properties)
{
	//若当前脚本类型符合函数调用
	if (Script_type == engine::Lua_type::Dynamic)
	{
		//运行函数名存储
		string function_name = "act";
		// ================================================================
		// 第一步：在 Lua 全局表中查找目标函数
		// ================================================================
		lua_getglobal(state_, function_name.c_str());

		if (!lua_isfunction(state_, -1))
		{
			lua_pop(state_, 1);
			return LuaTable();   // 返回无效对象
		}

		// ================================================================
		// 第二步：构造两个独立的参数 table
		// ================================================================

		// --- 第一个 table：run_data_slot（持久运行数据）---
		lua_newtable(state_);
		int slot_table_index = lua_gettop(state_);
		for (const auto& [key, value] : run_data_slot)
		{
			lua_pushstring(state_, key.c_str());
			lua_pushnumber(state_, value);
			lua_settable(state_, slot_table_index);
		}

		// --- 第二个 table：properties（本次调用传入的额外属性）---
		lua_newtable(state_);
		int prop_table_index = lua_gettop(state_);
		for (const auto& [key, value] : properties)
		{
			lua_pushstring(state_, key.c_str());
			lua_pushnumber(state_, value);
			lua_settable(state_, prop_table_index);
		}

		// 现在栈底是函数，然后是 slot_table，顶部是 prop_table

		// ================================================================
		// 第三步：调用函数，传入两个参数，期待一个返回值
		// ================================================================
		if (lua_pcall(state_, 2, 1, 0) != LUA_OK)
		{
			// 弹出错误信息
			lua_pop(state_, 1);
			return LuaTable();
		}

		// ================================================================
		// 第四步：处理返回值（期望是一个 table）
		// ================================================================
		LuaTable result;
		if (lua_istable(state_, -1))
			result = LuaTable(state_, -1);
		// 弹出返回值
		lua_pop(state_, 1);
		return result;
	}
}
LuaTable LuaScript::act(void)
{
	//若当前脚本类型符合函数调用
	if (Script_type == engine::Lua_type::Static)
	{
		//运行函数名存储
		string function_name = "act";
		// ================================================================
		// 第一步：在 Lua 全局表中查找目标函数
		// ================================================================
		lua_getglobal(state_, function_name.c_str());

		// 检查栈顶是不是一个函数
		if (!lua_isfunction(state_, -1))
		{
			//弹出非目标值
			lua_pop(state_, 1);
			return LuaTable();
		}

		// ================================================================
		// 第二步：把 C++ 的 unordered_map 转换成 Lua table
		// ================================================================
		lua_newtable(state_);
		//记录这个 table 在栈里的位置
		int table_index = lua_gettop(state_);

		//将已存储的信息槽压入
		for (const auto& [key, value] : run_data_slot)
		{
			// 压入键（字符串）
			lua_pushstring(state_, key.c_str());
			// 压入值（浮点数）
			lua_pushnumber(state_, value);
			//把键值对写入 table
			lua_settable(state_, table_index);
		}

		// 现在栈的状态是：底部是函数，顶部是 table
		// 函数在下，table 在上

		// ================================================================
		// 第三步：调用函数，把 table 作为参数传入
		// ================================================================
		if (lua_pcall(state_, 1, 1, 0) != LUA_OK)
		{
			// 函数执行出错了，错误信息在栈顶
			string error = lua_tostring(state_, -1);
			// 弹出错误信息
			lua_pop(state_, 1);
			return LuaTable();
		}

		// ================================================================
		// 第四步：处理返回值
		// ================================================================
		LuaTable result;
		if (lua_istable(state_, -1))
			result = LuaTable(state_, -1);
		lua_pop(state_, 1);
		return result;
	}
}