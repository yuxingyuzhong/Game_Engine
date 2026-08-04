#include "common/引擎总头文件.h"

int main(void)
{
	// 输出切到 UTF-8（调试输出仍用控制台）
	SetConsoleOutputCP(CP_UTF8);
	// 输入切到 UTF-8
	SetConsoleCP(CP_UTF8);

	//引入全部名称空间
	using namespace engine;

	//事件中心初始化
	Event_Broker event_broker;
	//配置加载器初始化
	Config_Loader config_loader;
	//实体管理器初始化
	Entity_Manager entity_Manager;

	//事件中转站依赖封装
	auto sign_up = [&event_broker](const std::string& name,
		const std::vector<config_event>& events,
		std::function<void(std::shared_ptr<config_event>)> event_entry)
		{
			event_broker.sign_up(name, events, event_entry);
		};
	auto receive = [&event_broker](std::vector<std::shared_ptr<config_event>> event_set)
		{
		event_broker.receive(event_set);
		};

	//实体管理器依赖注入
	entity_Manager.register_event_interface(sign_up,receive);

	//配置加载器依赖注入
	config_loader.event_entry_sign(receive);
	//获取可执行文件目录
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	//输入可执行文件绝对路径
	config_loader.exe_path_sign(buffer);
	//加载配置
	config_loader.act();
}
