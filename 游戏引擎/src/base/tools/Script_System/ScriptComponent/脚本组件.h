#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "src/base/tools/Script_System/LuaScript/Lua脚本.h"

//脚本系统模块
namespace Game_Engine
{
	class ScriptComponent
	{
	public:
		//添加脚本
		bool add_script(const string& script_name, const string& script_path)
		{
			//简化表示路径
			auto script = make_unique<LuaScript>();
			//错误信息存储
			string error;
			//若脚本加载失败则返回
			if (!script->load_script(script_path, error))
				return false;
			//若脚本类型不合法则返回
			if (script->script_type_get() == Lua_type::None)
				return false;

			//将脚本装载至容器
			script_set[script_name] = move(script);
			return true;
		}

		//添加脚本
		bool add_script(const string& script_name, const LuaScript& script);

		//卸载脚本
		void unload_script(const string& script_name)
		{
			//从容器中移除指定脚本，自动析构对应的 LuaScript
			script_set.erase(script_name);
		}

		//调用所有脚本进行逻辑处理
		void act(unordered_map<string, double>& custom_properties)
		{
			for (auto& [name, script] : script_set)
			{
				//获取当前脚本类型
				Lua_type Script_type = script->script_type_get();
				//若为动态脚本则加载数据槽
				if (Script_type == Lua_type::Dynamic)
					script->act(custom_properties);
				//若为静态脚本则直接执行
				else if (Script_type == Lua_type::Static)
					script->act();

				//若脚本状态为不存在则卸载脚本
				if (script->is_script_alive() == false)
					unload_script(name);
			}
		}

	private:
		//脚本存储容器
		unordered_map<string, unique_ptr<LuaScript>> script_set;
	};
}
