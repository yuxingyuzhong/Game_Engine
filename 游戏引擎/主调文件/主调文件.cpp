#include "common/引擎总头文件.h"

int main(void)
{
	// 输出切到 UTF-8（调试输出仍用控制台）
	SetConsoleOutputCP(CP_UTF8);
	// 输入切到 UTF-8
	SetConsoleCP(CP_UTF8);

	//引入全部名称空间
	using namespace engine;

	//事件中转站
	Event_Broker event_broker;
	//配置加载器
	Config_Loader config_loader;
	//属性槽管理器
	Property_Manager prop_manager;
	//效应管理器
	Effect_Manager effect_manager;
	//实体管理器
	Entity_Manager entity_manager;

	// ———— 事件中转站提供依赖封装 ————

	//接入入口封装
	auto attach_entry = [&event_broker](const std::string& name,
		const std::vector<config_event>& events,
		std::function<void(std::shared_ptr<config_event>)> event_entry)
		{
			event_broker.info_register(name, events, event_entry);
		};
	//事件入口封装
	auto event_entry = [&event_broker](std::vector<std::shared_ptr<config_event>> event_set)
		{
		event_broker.receive(event_set);
		};

	// ———— 属性槽管理器初始化 ———— 

	//接入入口注入
	prop_manager.event_terminal.attach_entry_register(attach_entry);
	//接入事件中转站
	prop_manager.attach();
	//属性槽获取通道封装
	auto prop_bind_entry = [&prop_manager](const uint64_t& ID) -> std::unordered_map<std::string, double>*
		{
			return prop_manager.prop_slot_get(ID);
		};

	// ———— 效应管理器初始化 ————

	//接入入口注入
	effect_manager.event_terminal.attach_entry_register(attach_entry);
	//事件入口注入
	effect_manager.event_terminal.event_entry_register(event_entry);
	//接入事件中转站
	effect_manager.attach();
	//属性槽获取通道注入
	effect_manager.bind_entry_register(prop_bind_entry);

	// ———— 实体管理器初始化 ————

	//接入入口注入
	entity_manager.event_terminal.attach_entry_register(attach_entry);
	//事件入口注入
	entity_manager.event_terminal.event_entry_register(event_entry);
	//接入事件中转站
	entity_manager.attach();
	//属性槽获取通道注入
	entity_manager.bind_entry_register(prop_bind_entry);

	// ———— 配置加载器初始化 ————
	
	//事件入口注入
	config_loader.event_terminal.event_entry_register(event_entry);
	//加载配置
	config_loader.act();

	for (;;)
	{

	}
}
