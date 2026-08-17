#include "../局部命名空间使用.h"

namespace engine
{
	//权限密钥生成
	int64_t Event_Terminal::acl_key_gen(void)
	{
		//若当前尚未生成密钥
		if (acl_key == 0)
		{
			//无限循环保证密钥成功生成
			for (;;)
			{
				//生成密钥
				acl_key = key_generator();
				//若密钥成功生成则返回
				if (acl_key != 0)
					return acl_key;
			}
		}
		//若当前已经生成密钥则返回无效值
		else
			return 0;
	}

	//函数包装器内存分配
	template <typename T>
	bool Event_Terminal::memory_malloc(std::unique_ptr<std::function<void(T parameter)>>& target)
	{
		//为目标对象分配内存
		target.reset(new(nothrow) function<void(T)>);
		//若内存分配失败
		if (!target)
			return false;
		//若内存分配成功
		else
			return true;
	}

	//函数接口注册
	template <typename T>
	bool Event_Terminal::function_register(std::unique_ptr<std::function<void(T parameter)>>& target,
		std::function<void(T parameter)> function)
	{
		//若内存分配成功
		if (memory_malloc(target))
		{
			//注册函数接口
			*(target) = function;
			//返回注册成功
			return true;
		}
		//若内存分配失败
		else
			//返回注册失败
			return false;
	}

	//事件发送入口注册 —— 单事件重载
	bool Event_Terminal::send_entry_register
	(function<void(shared_ptr<config_event> events)>event_send_entry)
	{
		//注册单事件发送入口
		return function_register(this->event_send_entry,event_send_entry);
	}
	
	//事件发送入口注册 —— 多事件重载
	bool Event_Terminal::send_entry_register
	(function<void(vector<shared_ptr<config_event>> events)>events_send_entry)
	{
		//注册多事件发送入口
		return function_register(this->events_send_entry, events_send_entry);
	}

	//事件接收入口注册 —— 单事件重载
	bool Event_Terminal::receive_entry_register
	(function<void(shared_ptr<config_event> event)>event_receive_entry)
	{
		//注册单事件接收入口
		return function_register(this->event_receive_entry, event_receive_entry);
	}

	//事件接收入口注册 —— 多事件重载
	bool Event_Terminal::receive_entry_register
	(function<void(vector<shared_ptr<config_event>> events)>events_receive_entry)
	{
		//注册多事件接收入口
		return function_register(this->events_receive_entry, events_receive_entry);
	}

	//接入入口注册
	bool Event_Terminal::attach_entry_register(function<void(const string& module_name,
		const vector<config_event>& needed_events,
		function<void(shared_ptr<config_event> evt)> receive_entry)> attach_entry)
	{
		//为接入入口分配内存
		this->attach_entry.reset(new(nothrow) function
			<void(const string & module_name,
				const vector<config_event>&needed_events,
				function<void(shared_ptr<config_event> evt)> receive_entry)>);
		//若内存分配失败
		if (!this->attach_entry)
			return false;
		else
			*(this->attach_entry) = attach_entry;

		return true;
	}

	//中转站接入
	bool Event_Terminal::attach(const string& module_name, const vector<config_event>& needed_events,
		const int64_t& acl_key)
	{
		//若密钥权限未匹配
		if (this->acl_key != acl_key)
			return false;

		//若未注册单事件接收入口
		if(!event_receive_entry)
		{
			//包装事件接收入口
			auto event_receive_entry = [this](shared_ptr<config_event> evt) -> void
				{
					this->event_receive(evt);
				};
			//调用事件接收重载
			(*attach_entry)(module_name, needed_events, event_receive_entry);
		}
		//若已注册单事件接收啊
		else
			//调用事件接收重载
			(*attach_entry)(module_name, needed_events, *event_receive_entry);

		//若事件接收入口注册完毕
		return true;
	}

	//事件发送 —— 单事件重载
	bool Event_Terminal::event_send(std::shared_ptr<config_event> event, const int64_t& acl_key)
	{
		//若权限密钥匹配
		if (this->acl_key == acl_key)
		{
			//若事件发送入口已激活
			if (event_send_entry)
			{
				//发送事件
				(*event_send_entry)(event);
				//返回发送成功
				return true;
			}
			else
				return false;
		}
		//若密钥不匹配则发送失败
		else
			return false;
	}

	//事件发送 —— 多事件重载
	bool Event_Terminal::event_send(vector<shared_ptr<config_event>> events, const int64_t& acl_key)
	{
		//若权限密钥匹配
		if (this->acl_key == acl_key)
		{
			//若事件发送入口已激活
			if (events_send_entry)
			{
				//发送事件
				(*events_send_entry)(events);
				//返回发送成功
				return true;
			}
			else
				return false;
		}
		//若密钥不匹配则发送失败
		else
			return false;
	}

	//事件接收 —— 单事件重载
	void Event_Terminal::event_receive(shared_ptr<config_event> event)
	{
		//若事件接收入口已额外注册
		if (event_receive_entry)
			(*event_receive_entry)(event);
		//若未额外注册则使用原生通道
		else
			event_set.push_back(event);
	}

	//事件接收 —— 多事件重载
	void Event_Terminal::event_receive(std::vector<std::shared_ptr<config_event>> events)
	{
		//若事件接收入口已额外注册
		if (events_receive_entry)
			(*events_receive_entry)(events);
		//若未额外注册则使用原生通道
		else
			event_set.insert(event_set.end(), events.begin(), events.end());
	}

	//事件查阅
	const vector<shared_ptr<config_event>>& Event_Terminal::event_get(const int64_t& acl_key)
	{
		//若密钥匹配则发送事件集合
		if (this->acl_key == acl_key)
			return event_set;
		else
			return {};
	}

	//事件清空
	bool Event_Terminal::clear(const int64_t& acl_key)
	{
		//若密钥匹配则清空所有事件
		if (acl_key == this->acl_key)
		{
			event_set.clear();
			//返回清空成功
			return true;
		}
		//返回无权清空
		else
			return false;
	}

}
