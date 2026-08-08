#pragma once
#include "common/前置头文件包含.h"

namespace engine
{
	//Lua表类型别名
	using LuaTable = sol::table;
	//Lua函数类型别名
	using LuaScript = sol::function;
	//Lua状态机类型别名
	using LuaState = sol::state;
}