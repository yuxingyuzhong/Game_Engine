#include"src/base/entity/Dynamic_Entity/局部命名空间使用.h"

//构造函数
Dynamic_Entity::Dynamic_Entity(const uint64_t& ID, const LuaScript& intialize_method,
	const LuaScript& action_method)
{
	//设置实体ID编号
	this->ID = ID;
	//注册初始化脚本
	utility_scripts.add_script("initialize", intialize_method);
	//运行初始化脚本
	utility_scripts.act(custom_properties);
	//卸载初始化脚本
	utility_scripts.unload_script("initialize");
	//注册行为决策脚本
	action_script.copy_script(action_method);
}

//行动
void Dynamic_Entity::act(void)
{
	//执行效用脚本
	utility_scripts.act(custom_properties);

	//执行行为脚本
	action_script.act(custom_properties);

}

//脚本接收
void Dynamic_Entity::srcipt_receive(const string& script_name,const LuaScript& script)
{
	//注册脚本
	utility_scripts.add_script(script_name,script);
}

//事件发送接口注册
void Dynamic_Entity::sign_up_entry(function<void(entity_event* event, vector<weak_ptr<Dynamic_Entity>>& entities)> entry)
{
	event_entry = entry;
}

//事件接收
void Dynamic_Entity::event_receive(config_event* evt)
{
	//记录新事件
	event_set.push_back(evt);
}

//事件发送
void Dynamic_Entity::event_send(entity_event* event)
{
	//发送事件
	event_entry(event, minions);
}
