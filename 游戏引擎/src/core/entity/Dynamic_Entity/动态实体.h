#pragma once
#include "common/前置头文件包含.h"
#include "src/core/entity/Entity_Manager/实体管理器通信结构体.h"
#include "src/core/entity/Entity/实体.h"
#include "src/core/Script_System/EffectComponent/效应组件.h"
#include "src/core/Script_System/SkillWrapComponent/技能参数打包组件.h"
#include "src/base/tools/Auxi_Algorithm/sol类型注册.h"

//游戏引擎命名空间
namespace engine
{
	//动态实体
	class Dynamic_Entity : public Entity
	{
	private:
		using LuaTable = sol::table;
		using LuaState = sol::state;
	private:
		//通用属性槽
		std::unordered_map<std::string, double> property_slot;

		//定义从属标签
		using minion_tag = std::pair<int64_t, std::string>;
		//定义从属
		using minion = std::weak_ptr<Dynamic_Entity>;

		//从属标签槽--脚本端
		std::unique_ptr<std::vector<minion_tag>> minion_tag_slot;
		//从属集合--引擎端
		std::unique_ptr<std::vector<minion>> minion_set;

		//从属转移缓冲--脚本端
		std::unique_ptr<std::vector<uint64_t>> transfer_buffer;
		//从属转移缓冲--引擎端
		std::unique_ptr<std::vector<minion>> entity_buffer;

		//事件集合
		std::vector<std::shared_ptr<config_event>> event_set{};
		//事件发送入口
		std::function<void(std::shared_ptr<config_event> event, std::vector<minion>& entities)>
			event_entry;

		//行为脚本
		LuaState action_script;
		//效应脚本
		std::unique_ptr<EffectComponent> effect_scripts;
		//技能参数打包脚本
		std::unique_ptr<SkillWrapComponent> wrap_scripts;

	public:
	    // ———— 对象初始化 ————
		
		//构造函数
		Dynamic_Entity(const int64_t& ID,LuaState& intialize_script);
		//析构函数
		~Dynamic_Entity();

	    // ———— 行为决策 ————
		
		//行为决策
		virtual void act(void);

		// ———— 脚本相关 ————
		
		//行为决策脚本注册
		void action_script_sign(const LuaTable& table);

		// ———— 事件相关 ————
		
		//事件发送接口注册
		void event_entry_sign(std::function<void(std::shared_ptr<config_event> event, 
			std::vector<minion>& entities)> entry);

		//事件接收
		void event_receive(std::shared_ptr<config_event> evt);
	private:
		//事件发送
		void event_send(std::shared_ptr<config_event> event);
	};

}