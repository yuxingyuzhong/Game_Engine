#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "src/base/entity/Entity_Manager/实体管理器通信结构体.h"
#include "src/base/entity/Entity/实体.h"
#include "src/base/tools/Script_System/ScriptComponent/脚本组件.h"

//游戏引擎命名空间
namespace Game_Engine
{
	//动态实体
	class Dynamic_Entity : public Entity
	{
	private:
		//事件集合
		vector<config_event*> event_set{};
		//通用属性槽
		unordered_map<string, double> custom_properties;
		//效用脚本
		ScriptComponent utility_scripts;
		//行为脚本
		LuaScript action_script;
		//从属集合
		vector<weak_ptr<Dynamic_Entity>> minions;
		//事件入口
		function<void(entity_event* event, vector<weak_ptr<Dynamic_Entity>>& entities)> 
			event_entry;
	public:
		//构造函数
		Dynamic_Entity(const uint64_t& ID,const LuaScript& intialize_method,
			const LuaScript& action_method);
		//析构函数
		~Dynamic_Entity() = default;
		//行动
		virtual void act(void);
		//脚本接收
		void srcipt_receive(const string& script_name,const LuaScript& script);
		//事件发送接口注册
		void sign_up_entry(function<void(entity_event* event, vector<weak_ptr<Dynamic_Entity>>& entities)> entry);
		//事件接收
		void event_receive(config_event* evt);
	private:
		//事件发送
		void event_send(entity_event* event);
	};

}