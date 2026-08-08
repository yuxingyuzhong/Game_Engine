#pragma once
#include "common/前置头文件包含.h"
#include "common/types/事件类型.h"
#include "src/core/event/Event_Terminal/事件终端.h"
#include "src/core/entity/Entity/实体.h"
#include "src/core/ScriptComponents/EffectComponent/效应组件.h"
#include "src/core/ScriptComponents/SkillWrapComponent/技能参数打包组件.h"
#include "common/external/Sol2/sol类型注册.h"
#include "common/external/Sol2/sol类型别名.h"


//游戏引擎命名空间
namespace engine
{
	//动态实体
	class Dynamic_Entity : public Entity
	{
	private:
		//定义从属标签
		using minion_tag = std::pair<int64_t, std::string>;
		//定义从属
		using minion = std::weak_ptr<Dynamic_Entity>;
		//脚本类定义
		struct ScriptClass
		{
			//脚本类名
			std::string name;              
			//该类的脚本实例列表
			std::vector<LuaState> scripts;       
		};
	private:
		//通用属性槽
		std::unordered_map<std::string, double> property_slot;

	    // ———— 从属相关 ————
		
		//从属标签槽--脚本端
		std::unique_ptr<std::vector<minion_tag>> minion_tag_slot;
		//从属集合--引擎端
		std::unique_ptr<std::vector<minion>> minion_set;

		//从属转移缓冲--脚本端
		std::unique_ptr<std::vector<uint64_t>> transfer_buffer;
		//从属转移缓冲--引擎端
		std::unique_ptr<std::vector<minion>> entity_buffer;

		// ———— 事件相关 ———— 
	public:
		//事件终端
		Event_Terminal event_terminal;
	private:
		//权限密钥
		int64_t acl_key = 0;

        // ———— 脚本相关 ————
		
		//行为脚本
		LuaState action_script;
		//效应脚本集合
		std::unique_ptr<EffectComponent> effect_scripts;
		//技能参数打包脚本集合
		std::unique_ptr<SkillWrapComponent> wrap_scripts;

		//其余脚本集合 —— 预留钩子
		std::vector<ScriptClass> other_scripts;

	public:
		//构造函数
		Dynamic_Entity(const int64_t& ID,LuaState& intialize_script);
		//析构函数
		~Dynamic_Entity();

	    // ———— 行为决策 ————
		
		//行为决策
		virtual void act(void);

		// ———— 脚本相关 ————
		
		//行为决策脚本注册
		void action_script_register(const LuaTable& table);

		// ———— 事件相关 ————

	private:
		//事件仲裁
		void event_govern(std::shared_ptr<config_event> event);
	};

}