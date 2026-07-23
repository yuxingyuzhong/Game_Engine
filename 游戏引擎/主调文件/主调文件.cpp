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
	auto sign_up = [&event_broker](const string& name,
		const vector<config_event>& events,
		function<void(config_event*)> event_entry) {
			event_broker.sign_up(name, events, event_entry);
		};
	auto receive = [&event_broker](vector<config_event*> event_set) {
		event_broker.receive(event_set);
		};
	//实体管理器依赖注入
	entity_Manager.sign_event_entry(sign_up,receive);
	//配置加载器依赖注入
	config_loader.sign_event_entry(receive);
	//加载配置
	config_loader.act();
}

/*
Tester tester;
	//使用功能测试器
	using engine::Tester;
	//进行四叉树单点查找测试
	//tester.qurdtree_block_seek_test(256, 128, 1024, 16, { 0.5,0.5 }, 500);
	//进行四叉树范围查找测试
	//tester.qurdtree_range_seek_test(256, 512, 512, 16, { 0.5,0.5 }, 100);
	//进行四叉树管理器单点查找测试
	tester.manager_block_seek_test(256,512,1024, 2048, 16, { 0.5,0.5 }, 1000);
	//进行四叉树管理器范围查找测试
	//tester.manager_range_seek_test(256, 1024, 128, 1024, 16, { 0.5,0.5 }, 100);
	//进行四叉树管理器智能创建合并测试
	//tester.manager_tree_build_merge_test(256,1024,8192,16384,16,10000);
*/