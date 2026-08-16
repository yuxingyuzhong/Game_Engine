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

		//更新接入信息
		event_terminal.attach("Effect_Manager", needed_events, receive_entry, acl_key);
	}

	//效应添加
	bool Effect_Manager::effect_build(std::shared_ptr<config_event> event)
	{
		//简化表示路径
		auto& config = event->config;

		//检验效应归属
		if (!config_checker.field_check<uint64_t>(config, "inclusion"))
			return false;

		//新效应索引记录
		uint64_t new_effect_index;
		//若空闲索引集合不为空
		if (!free_index_set.empty())
		{
			//获取空闲索引
			new_effect_index = free_index_set.back();
			//设置记录有效
			effect_set[new_effect_index].is_vaild = true;
			//删除该空闲索引
			free_index_set.pop_back();
		}
		else
		{
			//构造新效应记录
			effect_set.push_back({});
			//获取新效应索引
			new_effect_index = effect_set.size() - 1;
		}
			
		//获取新效应记录
		auto& new_record = effect_set[new_effect_index];
		//获取新效应
		auto& new_effect = new_record.pro_effect;

		//若效应配置解析异常
		if (!new_effect.config_read(config_checker, config))
		{
			//记录空闲索引
			free_index_set.push_back(new_effect_index);
			//设置记录无效
			new_record.is_vaild = false;
			return false;
		}
		//若效应配置解析正常
		else
		{
			//记录效应归属
			new_record.inclusion = config["inclusion"].get<uint64_t>();
			//绑定效应修改对象
			new_record.pro_effect.effect_object_bind(bind_entry(new_record.inclusion));

			//分配效应ID
			if (!recycle_ID_set.empty())
			{
				new_record.ID = recycle_ID_set.back();
				recycle_ID_set.pop_back();
			}
			else
				new_record.ID = start_ID++;
			//绑定效应ID
			new_record.pro_effect.ID_bind(new_record.ID);

			//获取事件终端
			auto& terminal = new_effect.event_terminal;
			//构造事件入口
			auto event_entry = [this](std::vector<std::shared_ptr<config_event>> events)->void
				{
					this->event_terminal.event_receive(events);
				};
			//配置事件终端
			terminal.event_entry_register(event_entry);

			//匹配效应分组
			for (auto& group : group_effect_set)
			{
				//若成功匹配效应分组
				if (group.inclusion == new_record.inclusion)
				{
					//配置效应信息
					event->config["ID"] = new_record.ID;

					//发送事件
					for (auto& effect : group.effects)
						effect->pro_effect.event_terminal.event_receive(event);

					//将新建事件加入分组
					group.effects.push_back(&new_record);
					//结束匹配
					break;
				}
			}

			//效应重排序
			effect_reranking(new_record);
		}
	}

	//效应卸载
	bool Effect_Manager::effect_unload(std::shared_ptr<config_event> event)
	{
		//简化表示路径
		auto& config = event->config;
		//查找目标索引
		int64_t index = effect_seek(config["target_ID"]);
		//若返回索引无效
		if (index < 0)
			return false;
		//若返回索引有效
		else
		{
			//简化表示路径
			auto& target_record = effect_set[index];
			//匹配效应分组
			for (int group_time = 0; group_time < group_effect_set.size(); group_time++)
			{
				//简化表示路径
				auto& group = group_effect_set[group_time];
				//若成功匹配效应分组
				if (group.inclusion == effect_set[index].inclusion)
				{
					//简化表示路径
					auto& effects = group.effects;

					//配置效应信息
					event->config["inclusion"] = target_record.inclusion;
					event->config["name"] = target_record.pro_effect.effect_name_get();

					//匹配目标效应
					for (int match_time = 0; match_time < effects.size(); match_time++)
					{
						//简化表示路径
						auto& effect = effects[match_time];

						//若为目标效应则销毁其记录
						if (effect->ID == effect_set[index].ID)
							effects.erase(effects.begin() + match_time);
						//若非目标效应则发送效应销毁事件
						else
							effect->pro_effect.event_terminal.event_receive(event);

						//若分组为空则销毁该分组
						if (effects.empty())
							group_effect_set.erase(group_effect_set.begin() + group_time);

						//结束匹配
						break;
					}
					//结束匹配
					break;
				}
			}

			//设置目标效应记录无效
			target_record.is_vaild = false;
			//回收目标效应ID
			recycle_ID_set.push_back(target_record.ID);
			//记录目标效应索引空闲
			free_index_set.push_back(index);
			//取消目标效应索引映射
			effect_index_map.erase(target_record.ID);
			//若目标效应为绝对优先执行效应
			if (!target_record.pro_effect.priority_get().has_value())
			{
				//将其与绝对优先执行效应末端效应交换位置
				swap(target_record, effect_set[abso_prior_index_end - 1]);
				//更新绝对优先执行效应边界
				abso_prior_index_end--;
			}
		}
		
		//返回卸载成功
		return true;
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

	//效应重排序
	void Effect_Manager::effect_reranking(const effect_record& new_record)
	{
		//获取新效应记录索引
		int64_t index = effect_seek(new_record.ID);
		//若新效应为绝对优先执行效应
		if (!new_record.pro_effect.priority_get().has_value())
		{
			//将新效应置于绝对优先执行效应边界
			swap(effect_set[abso_prior_index_end], effect_set[index]);
			//更新绝对优先执行效应边界
			abso_prior_index_end++;
		}
		else
			//根据执行优先级对非绝对优先执行效应降序排序
			sort(effect_set.begin() + abso_prior_index_end, effect_set.end(),
				greater(), [](const effect_record& record) { return record.pro_effect.priority_get(); });
	}

	//效应执行
	void Effect_Manager::run_effects(EffectPhase phase)
	{
		//执行效应
		for (auto& effect_record:effect_set)
		{
			//若记录有效
			if(!effect_record.is_vaild)
			{
				//简化表示路径
				auto& effect = effect_record.pro_effect;
				//若执行阶段标记匹配
				if (has_phase((EffectPhase)effect.phase_mask_get(), phase))
					effect.effect_act();
			}
		}
	}

	//外部事件处理
	void Effect_Manager::outer_event_process(std::shared_ptr<config_event> event)
	{

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
				//若为卸载/交流事件
				if(event->tag == "Unload" || event->tag == "Interact")
				{
					//获取事件发起者索引
					int64_t sender_index = effect_seek(config["sender_ID"]);

					//获取事件目标索引 
					int64_t target_index = effect_seek(config["target_ID"]);
					//若返回索引无效
					if (target_index < 0)
						continue;

					//简化表示路径
					auto& target_effect = effect_set[target_index].pro_effect;
					auto& sender_effect = effect_set[sender_index].pro_effect;

					//若为卸载事件
					if(event->tag == "Unload")
					{
						//若为自身卸载事件
						if (sender_index == target_index)
							//移除目标效应
							effect_unload(event);
						//若事件目标仅可被自身卸载
						else if (!target_effect.priority_get().has_value())
							continue;
						//若事件发起者消除级别大于等于事件目标抗消除级别
						else if (sender_effect.dispel_levels_get() >=
							target_effect.undispel_levels_get())
							//移除目标效应
							effect_unload(event);
					}
					//若为交流事件
					else
						//将事件发送给目标效应
						sender_effect.event_terminal.event_receive(event);
				}
			}
		}

		//向外部发送事件
		event_terminal.event_send(events,acl_key);
	}
}

