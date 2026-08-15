#include "../局部命名空间使用.h"
#include "src/tools/Non_GUI/logging/日志系统.h"

namespace engine
{
	//构造函数
	Property_Manager::Property_Manager()
	{
		//生成权限密钥
		acl_key = event_terminal.acl_key_gen();
	}

	//析构函数
	Property_Manager::~Property_Manager()
	{

	}

	//事件中转站接入
	void Property_Manager::attach(void)
	{
		//订阅事件集合记录
		vector<config_event> needed_events;

		//构造配置加载事件
		needed_events.emplace_back("Property_Manager", "Config", "Load",json::object());
		//构造实体构建事件
		needed_events.emplace_back("", "Entity", "Build", json::object());
		//构造实体卸载事件
		needed_events.emplace_back("", "Entity", "Unload",json::object());

		//构造事件接收入口
		auto receive_entry = [this](shared_ptr<config_event> event)-> void
			{
				this->event_process(event);
			};
		//更新接入信息
		event_terminal.attach("Property_Manager", needed_events, receive_entry, acl_key);
	}

	//属性槽获取
	unordered_map<string, double>* Property_Manager::prop_slot_get(const uint64_t& ID)
	{
		//查找目标属性槽索引
		uint64_t index = prop_slot_seek(ID);
		//若返回索引无效
		if (index < 0)
			return nullptr;
		else
		{
			//获取目标属性槽引用
			auto& record = record_set[index];
			//返回包装引用
			return &(record.property_slot);
		}
	}

	//属性槽构建
	bool Property_Manager::prop_slot_build(const string& type, const uint64_t& ID)
	{
		//获取目标属性槽迭代器
		auto it = initialize_scripts.find(type);
		//若迭代器无效
		if (it == initialize_scripts.end())
		{
			Log::warn("未配置目标类型属性槽初始化方式");
			Log::warn("属性槽无法构建");
			return false;
		}

		//构造新纪录
		record_set.push_back({});
		//简化表示路径
		auto& record = record_set.back();
		auto& initialize_script = it->second;
		//记录属性槽归属实体类型
		record.type = type;
		//记录属性槽归属实体编号
		record.ID = ID;
		//向初始化脚本注册属性槽
		initialize_script.set("pros", sol::as_table(record.property_slot));
		//运行脚本初始化属性槽
		initialize_script["initialize"]();
	}

	//属性槽卸载
	bool Property_Manager::prop_slot_unload(const string& type, const uint64_t& ID)
	{
		//查找目标属性槽索引
		uint64_t index = prop_slot_seek(ID);
		//若返回索引无效
		if (index < 0)
		{
			Log::warn("未发现目标属性槽");
			Log::warn("属性槽无法卸载");
			return false;
		}
		//若返回索引有效
		else
		{
			//卸载目标属性槽
			record_set.erase(record_set.begin() + index);
			//返回卸载成功
			return true;
		}
	}

	//属性槽查找
	uint64_t Property_Manager::prop_slot_seek(const uint64_t& ID)
	{
		//返回查找结果
		return binary_search(record_set, ID, less(), &prop_record::ID);
	}

	//事件处理
	void Property_Manager::event_process(std::shared_ptr<config_event> event)
	{
		//简化表示路径
		auto& config = event->config;
		auto& category = event->category;
		auto& tag = event->tag;
		//若为配置事件
		if (category == "Config" && tag == "Load")
		{
			//若实体类型字段无效
			if (!config_checker.field_check<string>(config, "type"))
			{
				Log::warn("Prop_Manager::配置出错");
				Log::warn("Prop_Manager::未定义目标实体类型!!!");
				return;
			}
			//若初始化脚本读取路径字段无效
			if (!config_checker.field_check<string>(config, "initialize_path"))
			{
				Log::warn("Prop_Manager::配置出错");
				Log::warn("Prop_Manager::未定义初始化脚本加载路径!!!");
				return;
			}
			//若为重复配置
			if (!initialize_scripts.count(config["type"].get<string>()))
			{
				Log::warn("Prop_Manager::配置出错");
				Log::warn("Prop_Manager::检测到当前为重复配置!!!");
				return;
			}

			//创建配置记录并初始化脚本
			initialize_scripts[config["type"]].load_file(config["initialize_path"]);
		}
		//若为实体事件
		if (category == "Entity")
		{
			//若为实体构建/卸载事件
			if (tag == "Build" || tag == "Unload")
			{
				//若实体类型字段无效
				if (!config_checker.field_check<string>(config, "type"))
				{
					Log::warn("Prop_Manager::配置出错");
					Log::warn("Prop_Manager::未定义目标实体类型!!!");
					return;
				}
				//若实体ID字段无效
				if (!config_checker.field_check<string>(config, "initialize_path"))
				{
					Log::warn("Prop_Manager::配置出错");
					Log::warn("Prop_Manager::未定义目标实体ID!!!");
					return;
				}

				//若为属性槽构建事件
				if(tag == "Build")
                    prop_slot_build(config["type"], config["ID"]);
				//若为属性槽卸载事件
				else
					prop_slot_unload(config["type"], config["ID"]);
			}
		}
	}
}
