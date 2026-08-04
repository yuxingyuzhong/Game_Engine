#pragma once
#include "common/前置头文件包含.h"
#include "src/base/tools/Auxi_Algorithm/sol类型注册.h"

//脚本系统模块
namespace engine
{
	//技能参数打包组件
	class SkillWrapComponent
	{
		//脚本别名
		using LuaTable = sol::table;
		using LuaState = sol::state;

		//通用属性槽指针
		const std::unordered_map<std::string, double>* ptr_property;
		//打包脚本集合
		std::unordered_map<std::string, std::unique_ptr<LuaState>> script_set;

		// --- 打包脚本管理接口 ---
	public:
	    //脚本添加
		void add_script(const std::string& script_name,const std::string& script_path);
		//脚本移除
		void remove_script(const std::string& script_name);
		//脚本执行
		LuaTable recall_script(const std::string& script_name);
		//通用属性槽注册
		void property_slot_sign(const std::unordered_map<std::string, double>& property_slot);
		//行为决策脚本功能注册
		void function_sign(LuaState& action_script);
	};
}
