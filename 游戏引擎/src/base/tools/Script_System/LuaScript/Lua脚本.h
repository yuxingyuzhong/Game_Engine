#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "src/base/tools/Script_System/LuaTable/Lua表.h"

namespace Game_Engine
{
	//脚本类型枚举
	enum class Lua_type
	{
		//初始类型标识
		None,
		//静态类型脚本
		Static,
		//动态类型脚本
		Dynamic,
	};

	//Lua脚本
	class LuaScript
	{
	public:
		//构造函数：创建一个独立的 Lua 运行环境
		LuaScript();

		//析构：安全销毁 Lua 状态机
		~LuaScript();

		//复制脚本
		void copy_script(const LuaScript& script);

		//加载脚本文件
		bool load_script(const string& script_path, string& error_out);

		//运行数据槽装载
		void data_slot_loading(const unordered_map<string, double>& data_slot);

		//脚本类型获取
		Lua_type script_type_get(void);

		//脚本存在状态获取
		bool is_script_alive(void);

		//执行脚本函数
		LuaTable act(const unordered_map<string, double>& properties);
		LuaTable act(void);

	private:
		//私有Lua虚拟机实例
		lua_State* state_;
		//脚本类型标记
		Lua_type Script_type = Lua_type::None;
		//运行数据槽
		unordered_map<string, double> run_data_slot;
	};
}
