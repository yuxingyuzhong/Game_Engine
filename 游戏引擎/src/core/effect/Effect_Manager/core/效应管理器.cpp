#include "../局部命名空间使用.h"
#include "src/tools/Non_GUI/Logging/日志系统.h"

//引擎命名空间
namespace engine
{
	//构造函数
	Effect_Manager::Effect_Manager()
	{
		//获取权限密钥
		acl_key = event_terminal.acl_key_gen();
	}

	//注册属性槽绑定通道
	void Effect_Manager::bind_entry_register(std::function<std::unordered_map<std::string, double>*
		(const uint64_t& ID)> bind_entry)
	{
		this->bind_entry = bind_entry;
	}

	//事件中转站接入
	void Effect_Manager::attach(void)
	{
		//订阅事件集合记录
		vector<config_event> needed_events;

		//构造配置加载事件
		needed_events.emplace_back("Effect_Manager", "Config", "Load", json::object());
		//构造实体构建事件
		needed_events.emplace_back("", "Effect", "Build", json::object());
		//构造实体卸载事件
		needed_events.emplace_back("", "Effect", "Unload", json::object());
		//构建事件接收入口
		auto receive_entry = [this](shared_ptr<config_event> event)->void
			{
				this->outer_event_process(event);
			};
		//注册事件接收入口
		event_terminal.receive_entry_register(receive_entry);

		//更新接入信息
		event_terminal.attach("Effect_Manager", needed_events, acl_key);
	}

	//效应查找
	int64_t Effect_Manager::effect_seek(const uint64_t& ID)
	{
		//获取目标效应迭代器
		auto it = effect_index_map.find(ID);
		//若目标效应不存在
		if (it == effect_index_map.end())
		{
			Log::warn("Effect_Manager::目标效应不存在");
			//返回无效索引
			return -1;
		}

		//返回目标效应索引索引 
		return it->second;
	}

	//效应分组查找
	int64_t Effect_Manager::effect_group_seek(const uint64_t& inclusion)
	{
		return binary_search(effect_groups,inclusion,less(), &effect_group::inclusion);
	}

	//效应ID分配
	uint64_t Effect_Manager::effect_ID_assign(void)
	{
		//若回收ID集合非空
		if (!recycle_IDs.empty())
		{
			//缓冲待分配ID
			uint64_t buffer = recycle_IDs.back();
			//弹出该ID
			recycle_IDs.pop_back();
			//返回ID
			return buffer;
		}
		else
			return next_ID++;
	}

	//效应构建
	optional<uint64_t> Effect_Manager::effect_build(shared_ptr<config_event> event)
	{
		//简化表示路径
		auto& config = event->config;

		//若效应归属字段无效
		if (!config_checker.field_check<uint64_t>(config, "inclusion"))
		{
			Log::warn("Effect_Manager::未指定效应归属\n效应构建事件已驳回");
			return nullopt;
		}
		//若效应执行阶段字段无效
		if (!config_checker.field_check<uint64_t>(config, "act_phase"))
		{
			Log::warn("Effect_Manager::未指定效应执行阶段\n效应构建事件已驳回");
			return nullopt;
		}
		//若执行优先级字段非字符串和无符号整数
		if (!config_checker.field_check<string>(config, "priority") &&
			!config_checker.field_check<uint64_t>(config, "priority"))
		{
			Log::warn("Prop_Effect::未定义执行优先级字段\n效应无法加载");
			return false;
	    }

		//新效应索引记录
		uint64_t index;
		//若空闲索引集合不为空
		if (!free_indexs.empty())
		{
			//获取空闲索引
			index = free_indexs.back();
			//删除该空闲索引
			free_indexs.pop_back();
			//设置记录有效
			effect_set[index].is_vaild = true;
		}
		else
		{
			//构造新效应记录
			effect_set.push_back({});
			//获取新效应索引
			index = effect_set.size() - 1;
		}
			
		//获取新效应记录
		auto& new_record = effect_set[index];
		//获取新效应
		auto& new_effect = new_record.pro_effect;

		//若效应配置解析异常
		if (!new_effect.config_read(config_checker, config))
		{
			//记录空闲索引
			free_indexs.push_back(index);
			//设置记录无效
			new_record.is_vaild = false;
			//返回无效值
			return nullopt;
		}
		//若效应配置解析正常
		else
		{
			//记录效应归属
			new_record.inclusion = config["inclusion"].get<uint64_t>();
			//分配效应ID
			new_record.ID = effect_ID_assign();
			//绑定效应ID
			new_record.pro_effect.ID_bind(new_record.ID);
			//记录效应执行阶段 
			new_record.act_phase = hash<string>{}(config["act_phase"].get<string>());
			//记录效应执行优先级
			if(config["priority"].is_string() && config["priority"].get<string>() == "max")
				new_record.priority = (numeric_limits<uint64_t>::max)();
			else
				new_record.priority = config["priority"].get<uint64_t>();
			//绑定效应修改对象
			new_record.pro_effect.effect_object_bind(bind_entry(new_record.inclusion));
			//建立效应索引映射
			effect_index_map.insert({ new_record.ID,index });

			//获取事件终端
			auto& terminal = new_effect.event_terminal;
			//构造事件入口
			auto event_send_entry = [this](std::vector<std::shared_ptr<config_event>> events)->void
				{
					this->event_terminal.event_receive(events);
				};
			//配置事件终端
			terminal.send_entry_register(event_send_entry);

			//获取效应分组索引
			int64_t group_index = effect_group_seek(new_record.inclusion);
			//若返回索引无效
			if (group_index < 0)
			{
				//创建效应分组
				effect_groups.push_back({ new_record.inclusion ,{} });
				//设置效应分组索引
				group_index = effect_groups.size() - 1;
			}
			//获取目标效应分组
			auto& group = effect_groups[group_index];

			//配置效应信息
			event->config["ID"] = new_record.ID;
			//发送事件
			for (auto& effect : group.effects)
				effect->pro_effect.event_terminal.event_receive(event);
			//将新建效应加入分组
			group.effects.push_back(&new_record);

			//根据执行优先级降序排序
			sort(effect_set,greater(),&effect_record::priority);
			
			//返回新效应ID
			return new_record.ID;
		}
	}

	//效应卸载
	bool Effect_Manager::effect_unload(std::shared_ptr<config_event> event)
	{
		//简化表示路径
		auto& config = event->config;

		//若效应ID字段无效
		if (!config_checker.field_check<uint64_t>(config, "target_ID"))
		{
			Log::warn("Effect_Manager::效应ID未定义\n效应卸载事件已驳回");
			return false;
		}

		//获取效应ID
		uint64_t target_ID = config["target_ID"].get<uint64_t>();
		//查找目标索引
		int64_t index = effect_seek(target_ID);
		//若返回索引无效
		if (index < 0)
		{
			Log::warn("Effect_Manager::效应ID无意义\n效应卸载事件已驳回");
			return false;
		}
		
		//简化表示路径
		auto& target_record = effect_set[index];
		//获取效应分组索引
		int64_t group_index = effect_group_seek(target_record.inclusion);
		//获取效应分组
		auto& group = effect_groups[group_index];
		//获取效应集合
		auto& effects = group.effects;

		//若组内仅有目标效应
		if (effects.size() == 1)
			//卸载该效应分组
			effect_groups.erase(effect_groups.begin() + group_index);
		else
		{
			//配置效应信息
			event->config["inclusion"] = target_record.inclusion;
			event->config["name"] = target_record.pro_effect.effect_name_get();

			//目标效应索引记录
			int64_t target_index = -1;
			//匹配目标效应
			for (int match_time = 0; match_time < effects.size(); match_time++)
			{
				//简化表示路径
				auto& effect = effects[match_time];
				//若非目标效应则发送效应销毁事件
				if (effect->ID != target_record.ID)
					effect->pro_effect.event_terminal.event_receive(event);
				//若为目标效应则记录其索引
				else
					target_index = match_time;
			}

			//卸载组内效应记录
			effects.erase(effects.begin() + target_index);
		}

		//设置目标效应记录无效
		target_record.is_vaild = false;
		//回收目标效应ID
		recycle_IDs.push_back(target_record.ID);
		//记录目标效应索引空闲
		free_indexs.push_back(index);
		//取消目标效应索引映射
		effect_index_map.erase(target_record.ID);

		//返回卸载成功
		return true;
	}

	//效应执行
	void Effect_Manager::effect_act(uint64_t phase)
	{
		//执行效应
		for (auto& effect_record:effect_set)
		{
			//若记录有效
			if(effect_record.is_vaild)
			{
				//简化表示路径
				auto& effect = effect_record.pro_effect;
				//若执行阶段标记匹配
				if (effect_record.act_phase == phase)
					effect.effect_act();
			}
		}
	}

	//外部事件处理
	void Effect_Manager::outer_event_process(std::shared_ptr<config_event> event)
	{
		//若为效应大类分支
		if (event->category == "Effect")
		{
			//简化表示路径
			auto& tag = event->tag;
			auto& config = event->config;

			//若为效应构建事件
			if (tag == "Build")
			{
				//构建新效应
				optional<uint64_t> effect_ID = effect_build(event);
				//若效应构造成功
				if (effect_ID.has_value())
					config["effect_ID"] = effect_ID.value();
				//若效应构造失败则分配异常ID
				else
					config["effect_ID"] = -1;
				//全局发送修饰事件
				event_terminal.event_send(event, acl_key);
			}
			//若为效应卸载事件
			else if (tag == "Unload")
			{
				//处理效应卸载事件
				effect_unload(event);
			}
			//若为效应触发事件
			else if (tag == "Act")
			{
				//若效应执行阶段字段未定义
				if (!config_checker.field_check<string>(config, "act_phase"))
				{
					Log::warn("Effect_Manager::效应执行阶段未定义\n效应触发事件已驳回");
					return;
				}

				//获取效应执行时段
				uint64_t act_phase = hash<string>{}(config["act_phase"].get<string>());
				//触发符合时间段的效应
				effect_act(act_phase);
			}
			//若为其余事件
			else
			{
				//若效应ID字段未定义
				if (!config_checker.field_check<string>(config, "target_ID"))
				{
					Log::warn("Effect_Manager::目标效应ID未定义\n未知事件已驳回");
					return;
				}

				//获取目标效应ID
				uint64_t target_ID = config["target_ID"].get<uint64_t>();
				//获取效应索引映射迭代器
				auto it = effect_index_map.find(target_ID);
				//若迭代器有效
				if (it != effect_index_map.end())
					effect_set[it->second].pro_effect.event_terminal.event_receive(event);
				//若迭代器无效
				else
				{
					Log::warn("Effect_Manager::目标效应ID不存在\n未知事件已驳回");
					return;
				}
			}
		}
	}

	//内部事件仲裁
	void Effect_Manager::inner_event_govern(std::vector<std::shared_ptr<config_event>> events)
	{
		for (auto& event: events)
		{
			//若为效应大类分支
			if (event->category == "Effect")
			{
				//简化表示路径
				auto& config = event->config;
				//若为交流事件
				if(event->tag == "Interact")
				{
					//若事件发起者ID字段无效
					if (!config_checker.field_check<uint64_t>(config, "sender_ID"))
					{
						Log::warn("Effecr_Manager::事件发起者ID未定义\n效应交流事件已驳回");
						return;
					}
					//若事件目标ID字段无效
					if (!config_checker.field_check<uint64_t>(config, "sender_ID"))
					{
						Log::warn("Effecr_Manager::事件目标ID未定义\n效应交流事件已驳回");
						return;
					}

					//获取事件发起者索引
					int64_t sender_index = effect_seek(config["sender_ID"]);
					//获取事件目标索引 
					int64_t target_index = effect_seek(config["target_ID"]);
					//若返回索引无效
					if (sender_index < 0 || target_index < 0)
					{
						Log::warn("Effecr_Manager::事件发起者/目标ID无效\n效应交流事件已驳回");
						continue;
					}

					//简化表示路径
					auto& target_effect = effect_set[target_index].pro_effect;
					auto& sender_effect = effect_set[sender_index].pro_effect;

					//将事件发送给目标效应
					sender_effect.event_terminal.event_receive(event);
				}
			}
		}

		//向外部发送事件
		event_terminal.event_send(events,acl_key);
	}
}

