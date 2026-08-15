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

		//更新接入信息
		event_terminal.attach("Effect_Manager", needed_events, acl_key);
	}

	//效应添加
	bool Effect_Manager::effect_build(const json& config)
	{
		//检验效应归属
		if (!config_checker.field_check<uint64_t>(config, "inclusion"))
			return false;
		//若归属已指定
		else
		{
			//构造新效应
			effect_set.push_back({});
			//简化表示路径
			auto& new_record = effect_set.back();

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
		}

		//简化表示路径
		auto& new_record = effect_set.back();
		auto& new_effect = new_record.pro_effect;

		//若效应配置解析异常
		if(!new_effect.config_read(config_checker,config))
		{
			//卸载新效应
			effect_set.pop_back();
			return false;
		}
		//若效应配置解析正常
		else
		{			
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
					//构造效应构建事件
					shared_ptr<config_event> effect_added_event(new(nothrow) config_event);
					//若内存分配失败
					if (!effect_added_event)
						return false;

					//标记事件大类
					effect_added_event->category = "Prop_Effect";
					//标记事件标签
					effect_added_event->tag = "Added";
					//配置效应信息
					effect_added_event->config = config;
					effect_added_event->config["ID"] = new_record.ID;

					//发送事件
					for (auto& effect : group.effects)
						effect->pro_effect.event_terminal.event_receive(effect_added_event);

					//将新建事件加入分组
					group.effects.push_back(&new_record);
					//结束匹配
					break;
				}
			}

			//对效应集合升序排序
			sort(effect_set, less(), & effect_record::ID);
		}
	}

	//效应卸载
	bool Effect_Manager::effect_unload(const uint64_t& effect_ID)
	{
		//效应存储索引记录
		int64_t index = binary_search(effect_set, effect_ID, less(), &effect_record::ID);
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

					//构造效应构建事件
					shared_ptr<config_event> effect_unload_event(new(nothrow) config_event);
					//若内存分配失败
					if (!effect_unload_event)
						return false;

					//标记事件大类
					effect_unload_event->category = "Prop_Effect";
					//标记事件标签
					effect_unload_event->tag = "Unload";
					//配置效应信息
					effect_unload_event->config["ID"] = target_record.ID;
					effect_unload_event->config["inclusion"] = target_record.inclusion;
					effect_unload_event->config["name"] = target_record.pro_effect.effect_name_get();

					//匹配目标效应
					for (int match_time = 0;match_time < effects.size();match_time++)
					{
						//简化表示路径
						auto& effect = effects[match_time];

						//若为目标效应则销毁其记录
						if(effect->ID == effect_set[index].ID)
							effects.erase(effects.begin() + match_time);
						//若非目标效应则发送效应销毁事件
						else
							effect->pro_effect.event_terminal.event_receive(effect_unload_event);

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

			//从总集合中卸载该效应
			effect_set.erase(effect_set.begin() + index);
		}
		
		//返回卸载成功
		return true;
	}

	//效应执行
	void Effect_Manager::run_effects(EffectPhase phase)
	{
		//处理缓存事件
		inner_event_govern(event_terminal.event_get(acl_key));
		//执行效应
		for (auto& effect_record:effect_set)
		{
			//简化表示路径
			auto& effect = effect_record.pro_effect;
			//发送预发送事件
			effect.event_terminal.event_receive(pre_event_set);
			//若执行阶段标记匹配
			if (has_phase((EffectPhase)effect.phase_mask_get(), phase))
				effect.effect_act();
		}

		//处理缓存事件
		inner_event_govern(event_terminal.event_get(acl_key));
		//清空缓存事件
		event_terminal.clear(acl_key);
		//清空预发送事件
		pre_event_set.clear();
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
				//若为构建事件
				if (event->tag == "Build")
					effect_build(event->config);
				//若为卸载事件
				else if(event->tag == "Unload")
				{
					//事件发起者索引记录
					int64_t sender_index = binary_search(effect_set, config["sender_ID"].get<uint64_t>(),
						less(), &effect_record::ID);
					//若返回索引无效
					if (sender_index < 0)
						continue;

					//事件目标索引记录
					int64_t target_index = binary_search(effect_set, config["target_ID"].get<uint64_t>(),
						less(), &effect_record::ID);
					//若返回索引无效
					if (target_index < 0)
						continue;

					//简化表示路径
					auto& target_effect = effect_set[target_index].pro_effect;
					auto& sender_effect = effect_set[sender_index].pro_effect;

					//若为自身卸载事件
					if (sender_index == target_index)
						//移除目标效应
						effect_unload(target_index);
					//若事件目标仅可被自身卸载
					else if (!target_effect.priority_get().has_value())
						continue;
					//若事件发起者消除级别大于等于事件目标抗消除级别
					else if (sender_effect.dispel_levels_get() >=
						target_effect.undispel_levels_get())
						//移除目标效应
						effect_unload(target_index);

				}
				//若为交流事件
				else if (event->tag == "Interact")
					//记录该预发送事件
					pre_event_set.push_back(event);
			}
		}

		//向外部发送事件
		event_terminal.event_send(events,acl_key);
	}
}

