#include "../局部命名空间使用.h"

namespace engine
{
	// ———— 单事件重载 ————
	
	//事件发送入口注册
	bool Event_Terminal::event_entry_register
	(function<void(shared_ptr<config_event> events)>event_entry)
	{
		//为事件发送入口分配内存
		this->event_entry.reset(new(nothrow) function<void(shared_ptr<config_event>)>);
		//若内存分配失败
		if (!this->event_entry)
			return false;
		else
			*(this->event_entry) = event_entry;

		return true;
	}

	//事件发送入口注册 —— 额外参数重载
	bool Event_Terminal::event_entry_register
	(function<void(shared_ptr<config_event> events, any others)>event_entry)
	{
		//为事件发送入口分配内存
		this->ex_event_entry.reset(new(nothrow) function
			<void(shared_ptr<config_event> events, any others)>);
		//若内存分配失败
		if (!this->ex_event_entry)
			return false;
		else
			*(this->ex_event_entry) = event_entry;

		return true;
	}

	// ———— 多事件重载 ————
	
	//事件发送入口注册
	bool Event_Terminal::event_entry_register
	(function<void(vector<shared_ptr<config_event>> events)>event_entry)
	{
		//为事件发送入口分配内存
		this->events_entry.reset(new(nothrow) function<void(vector<shared_ptr<config_event>>)>);
		//若内存分配失败
		if (!this->events_entry)
			return false;
		else
			*(this->events_entry) = event_entry;

		return true;
	}

	//事件发送入口注册 —— 额外参数重载
	bool Event_Terminal::event_entry_register
	(function<void(vector<shared_ptr<config_event>> events,any others)>event_entry)
	{
		//为事件发送入口分配内存
		this->ex_events_entry.reset(new(nothrow) function
			<void(vector<shared_ptr<config_event>> events, any others)>);
		//若内存分配失败
		if (!this->ex_events_entry)
			return false;
		else
			*(this->ex_events_entry) = event_entry;

		return true;
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
		//包装事件接收入口
		auto receive_entry = [this](shared_ptr<config_event> evt) -> void
			{
				this->event_receive(evt);
			};
		//调用事件接收重载
		return attach(module_name, needed_events, receive_entry, acl_key);
	}

	//中转站接入 —— 事件接收入口设置重载
	bool Event_Terminal::attach(const std::string& module_name, const std::vector<config_event>& needed_events,
		std::function<void(std::shared_ptr<config_event> evt)> receive_entry,
		const int64_t& acl_key)
	{
		//若权限密钥匹配
		if (this->acl_key == acl_key)
		{
			//接入事件中转站
			(*attach_entry)(module_name, needed_events, receive_entry);
			//返回接入成功
			return true;
		}
		//若密钥不匹配则返回接入失败
		else
			return false;
	}

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

	// ———— 单事件重载 ————

	//事件发送
	bool Event_Terminal::event_send(std::shared_ptr<config_event> event, const int64_t& acl_key)
	{
		//若权限密钥匹配
		if (this->acl_key == acl_key)
		{
			//若事件发送入口已激活
			if (event_entry)
			{
				//发送事件
				(*event_entry)(event);
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

	//事件发送 —— 额外参数重载
	bool Event_Terminal::event_send(std::shared_ptr<config_event> event, std::any others, const int64_t& acl_key)
	{
		//若权限密钥匹配
		if (this->acl_key == acl_key)
		{
			//若事件发送入口已激活
			if (ex_event_entry)
			{
				//发送事件
				(*ex_event_entry)(event, others);
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

	// ———— 多事件重载 ————

	//事件发送
	bool Event_Terminal::event_send(vector<shared_ptr<config_event>> events, const int64_t& acl_key)
	{
		//若权限密钥匹配
		if (this->acl_key == acl_key)
		{
			//若事件发送入口已激活
			if (events_entry)
			{
				//发送事件
				(*events_entry)(events);
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

	//事件发送 —— 额外参数重载
	bool Event_Terminal::event_send(vector<shared_ptr<config_event>> events, any others, const int64_t& acl_key)
	{
		//若权限密钥匹配
		if (this->acl_key == acl_key)
		{
			//若事件发送入口已激活
			if (ex_events_entry)
			{
				//发送事件
				(*ex_events_entry)(events, others);
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
		event_set.push_back(event);
	}

	//事件接收 —— 多事件重载
	void Event_Terminal::event_receive(std::vector<std::shared_ptr<config_event>> events)
	{
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
