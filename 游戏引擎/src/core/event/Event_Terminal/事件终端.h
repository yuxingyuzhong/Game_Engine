#pragma once
#include "common/前置头文件包含.h"
#include "common/types/事件类型.h"
#include "src/tools/Random/随机数生成器.h"

//游戏引擎命名空间
namespace engine
{
	class Event_Terminal
	{
	private:
		//权限密钥
		int64_t acl_key = 0;
		//密钥生成器
		Random_Generator key_generator{};

		//事件集合
		std::vector<std::shared_ptr<config_event>> event_set{};

		//中转站接入入口
		std::function<void(const std::string& module_name, const std::vector<config_event>& needed_events,
			std::function<void(std::shared_ptr<config_event> evt)> event_entry)> attach_entry;

		//事件发送入口
		std::function<void(std::vector<std::shared_ptr<config_event>> events)> event_entry;
		//事件发送入口 —— 额外参数
		std::function<void(std::vector<std::shared_ptr<config_event>> events,std::any others)> extra_event_entry;

	public:
		//构造函数
		Event_Terminal() = default;

		//析构函数
		~Event_Terminal() = default;

        // ———— 功能激活 ————

		//接入入口注册
		void attach_entry_register(std::function<void(const std::string& module_name,
			const std::vector<config_event>& needed_events,
			std::function<void(std::shared_ptr<config_event> evt)> receive_entry)> attach_entry);

		//事件发送入口注册
		void event_entry_register
		(std::function<void(std::vector<std::shared_ptr<config_event>> events)>event_entry);

		//事件发送入口注册 —— 额外参数重载
		void event_entry_register
		(std::function<void(std::vector<std::shared_ptr<config_event>> events, std::any others)>event_entry);

		//权限密钥生成
		int64_t acl_key_gen(void);

		// ———— 中转站交互 ————

		//中转站接入
		bool attach(const std::string& module_name,const std::vector<config_event>& needed_events,
			const int64_t& acl_key);

		//中转站接入 —— 事件接收重载
		bool attach(const std::string& module_name, const std::vector<config_event>& needed_events,
			std::function<void(std::shared_ptr<config_event> evt)> receive_entry,
			const int64_t& acl_key);

		// ———— 事件交互 ————

		//事件发送
		bool event_send(std::vector<std::shared_ptr<config_event>> events,const int64_t& acl_key);

		//事件发送 —— 额外参数重载
		bool event_send(std::vector<std::shared_ptr<config_event>> events, std::any others, const int64_t& acl_key);

		//事件接收
		void event_receive(std::shared_ptr<config_event> event);

		//事件查阅
		const std::vector<std::shared_ptr<config_event>>& event_get(void);

		//事件清空
		bool clear(const int64_t& acl_key);
	};
}