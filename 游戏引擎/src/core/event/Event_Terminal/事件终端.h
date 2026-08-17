#pragma once
//预编译头
#include "common/前置头文件包含.h"

// ———— 事件相关 ————

//获取预定义事件类型
#include "common/types/事件类型.h"

// ———— 工具相关————

//获取随机数生成器(用于权限密钥生成)
#include "src/tools/Non_GUI/Random/随机数生成器.h"

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
		std::unique_ptr<std::function<void(const std::string& module_name,
			const std::vector<config_event>& needed_events,
			std::function<void(std::shared_ptr<config_event> evt)> event_send_entry)>>
			attach_entry;

		//事件发送入口 —— 单事件重载
		std::unique_ptr<std::function<void(std::shared_ptr<config_event> event)>> event_send_entry;
		//事件发送入口 —— 多事件重载
		std::unique_ptr<std::function<void(std::vector<std::shared_ptr<config_event>> events)>> events_send_entry;

		//事件接收入口 —— 单事件重载
		std::unique_ptr<std::function<void(std::shared_ptr<config_event> event)>> event_receive_entry;
		//事件接收入口 —— 多事件重载
		std::unique_ptr<std::function<void(std::vector<std::shared_ptr<config_event>> events)>> events_receive_entry;

	public:
		//构造函数
		Event_Terminal() = default;
		//析构函数
		~Event_Terminal() = default;
		//默认移动构造函数
		Event_Terminal(Event_Terminal&&) = default;
		//默认移动复制函数
		Event_Terminal& operator=(Event_Terminal&&) = default;

        // ———— 功能激活 ————

		//权限密钥生成
		int64_t acl_key_gen(void);

		//接入入口注册
		bool attach_entry_register(std::function<void(const std::string& module_name,
			const std::vector<config_event>& needed_events,
			std::function<void(std::shared_ptr<config_event> evt)> receive_entry)> attach_entry);

		//事件发送入口注册 —— 单事件重载
		bool send_entry_register
		(std::function<void(std::shared_ptr<config_event> events)>event_send_entry);

		//事件发送入口注册 —— 多事件重载
		bool send_entry_register
		(std::function<void(std::vector<std::shared_ptr<config_event>> events)>event_send_entry);

		//事件接收入口注册 —— 单事件重载
		bool receive_entry_register
		(std::function<void(std::shared_ptr<config_event> events)>event_receive_entry);
		//事件接收入口注册 —— 多事件重载
		bool receive_entry_register
		(std::function<void(std::vector<std::shared_ptr<config_event>> events)>event_receive_entry);

		// ———— 中转站交互 ————

		//中转站接入
		bool attach(const std::string& module_name,const std::vector<config_event>& needed_events,
			const int64_t& acl_key);

		// ———— 事件交互 ————

		//事件发送 —— 单事件重载
		bool event_send(std::shared_ptr<config_event> event, const int64_t& acl_key);
		
		//事件发送 —— 多事件重载
		bool event_send(std::vector<std::shared_ptr<config_event>> events,const int64_t& acl_key);

		//事件接收 —— 单事件重载
		void event_receive(std::shared_ptr<config_event> event);

		//事件接收 —— 多事件重载
		void event_receive(std::vector<std::shared_ptr<config_event>> events);

		//事件查阅
		const std::vector<std::shared_ptr<config_event>>& event_get(const int64_t& acl_key);

		//事件清空
		bool clear(const int64_t& acl_key);
	private:
		//函数包装器内存分配
	    template <typename T>
		bool memory_malloc(std::unique_ptr<std::function<void(T parameter)>>& target);
		//函数接口注册
		template <typename T>
		bool function_register(std::unique_ptr<std::function<void(T parameter)>>& target,
			std::function<void(T parameter)> function);
	};
}