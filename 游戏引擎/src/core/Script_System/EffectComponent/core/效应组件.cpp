#include "src/core/Script_System/EffectComponent/局部命名空间使用.h"

//引擎命名空间
namespace engine
{
	//字段初始化
	bool EffectComponent::field_initialize(unique_ptr<EffectSlot>& effect, const json& config)
	{
		//若效应读取路径未定义
		if (config.value<std::string>("effect_path", {}).empty())
			return false;
		else
			effect->script.load_file(config["effect_path"]);
		//若效应名称未定义
		if (config.value<std::string>("effect_name", {}).empty())
			return false;
		else
			effect->name = config["name"];
		//若效应分类未定义
		if (config.value<std::string>("effect_category", {}).empty())
			return false;
		else
			effect->group = config["category"];
		//若效应叠加上限未定义
		if (config.value<std::string>("max_stacks", {}).empty())
			return false;
		else
			effect->max_stacks = config["max_stacks"];
		//若效应执行优先级未定义
		if (config.value<std::string>("priority", {}).empty())
			return false;
		else
			effect->priority = config["priority"];
		//若效应执行次序未定义
		if (config.value<std::string>("phase_mask", {}).empty())
			return false;
		else
			effect->phase_mask = config["phase_mask"];

		//若所有字段均存在则返回true
		return true;
	}

	//同类效应更新
	void EffectComponent::homogeneous_effect_update(const uint64_t& leader_index, const uint64_t& new_index,
		bool replace)
	{
		//简化表示路径
		auto& leader_effect = effects[leader_index];
		auto& new_effect = effects[new_index];
		auto& others = leader_effect->others;

		//若新增效应优先级较领队效应高
		if (new_effect->priority < leader_effect->priority)
		{
			//将原领队效应加入链表
			others.push_front(leader_effect.get());
			//转移链表所有权
			new_effect->others = others;
			others = {};
			//若进行效应取代
			if (replace)
				new_effect->others.pop_back();
		}
		//若新增效应优先级较领队效应低
		else
		{
			//获取迭代器
			auto it = others.begin();
			//若迭代器未超尾或者新增效优先级较当前效应低
			while (it != others.end() && (*it)->priority < new_effect->priority)
				it++;
			//插入新增效应
			others.insert(it, new_effect.get());
			//若进行效应取代
			if (replace)
				others.pop_back();
		}
	}

	// --- 效应管理接口 ---

	//效应添加
	void EffectComponent::add_effect(const json& config)
	{
		//添加新效应
		effects.push_back({});
		//简化表示路径
		auto& new_effect = effects.back();

		//若字段可用则进行下一步操作
		if (field_initialize(new_effect, config))
		{
			//若当前效应所在分类已经存在
			if (effect_map_set.count(new_effect->group))
			{
				//寻找当前优先级最高同类效应
				//优先级越高，效应索引越低
				for (int seek_time = 0; seek_time < effects.size(); seek_time++)
				{
					//简化表示路径
					auto& leader_effect = effects[seek_time];
					//若发现优先级最高同类效应
					if (leader_effect->group == new_effect->group)
					{
						//效应堆叠次数记录
						int stack_counts = 1 + leader_effect->others.size();

						//若状态已经堆叠至上限
						if (stack_counts >= leader_effect->max_stacks)
							homogeneous_effect_update(seek_time, effects.size() - 1,
								true);
						//若状态堆叠未至上限
						else
							homogeneous_effect_update(seek_time, effects.size() - 1,
								false);
					}

				}
			}
			//若当前效应所在分类不存在
			else
				//插入新分组
				effect_map_set.insert({ new_effect->group, {} });

			//对新插入效应进行排序
			sort(effects, less(), &EffectSlot::priority);
		}
		//若字段不可用则直接返回
		else
		{
			//卸载新效应
			effects.pop_back();
			return;
		}
	}

	//效应移除
	void EffectComponent::remove_effect(const uint64_t& index)
	{
		//简化表示路径
		auto& unload_effect = effects[index];

		//若待卸载效应具有连带卸载特性
		if (unload_effect->script.get_or("cascade_unload", false))
		{
			//领队效应索引存储
			int leader_index = 0;

			//卸载领队效应持有链表
			for (int find_time = 0; find_time < effects.size(); find_time++)
			{
				//若发现领队效应
				if (effects[find_time]->group == unload_effect->group)
				{
					//记录领队效应索引
					leader_index = find_time;

					//简化表示路径
					auto& others = effects[find_time]->others;
					//获取待卸载效应迭代器
					auto it = find(others.begin(), others.end(), unload_effect.get());
					//若返回迭代器无效则待卸载效应即为领队效应
					if (it == others.end())
						//卸载所有节点
						others.clear();
					else
						//卸载部分节点
						others.erase(it, others.end());
					break;
				}
			}

			//连带卸载优先级较低同类效应
			for (int unload_index = effects.size() - 1; unload_index >= leader_index; unload_index++)
			{
				//简化表示路径
				auto& now_effect = effects[unload_index];
				//若当前效应与待卸载效应分类相同且优先级较低
				if (now_effect->group == unload_effect->group &&
					now_effect->priority > unload_effect->priority)
					effects.erase(effects.begin() + unload_index);
			}
		}

		//卸载待卸载脚本
		effects.erase(effects.begin() + index);
	}

	// --- 执行调度接口 ---

	//按阶段执行所有匹配的效应
	void EffectComponent::run_effects(EffectPhase phase)
	{
		for (auto& effect : effects)
		{
			//若执行阶段标记匹配
			if (effect.get()->phase_mask == phase)
				effect.get()->script["action"]();
		}
	}

	// --- 查询接口 ---

	//效应效果获取
	const EffectMap& EffectComponent::effect_work_get(const std::string& effect_name)
	{
		//若存在指定效应
		if (effect_map_set.count(effect_name))
			return effect_map_set[effect_name];
		//若不存在则返回空表
		else
			return {};
	}

	//修正属性获取
	optional<double> EffectComponent::property_modifier_get(const std::string& prop_key)
	{
		//若存在指定属性
		if (modifiers_property_cache.count(prop_key))
			return modifiers_property_cache[prop_key];
		//若不存在则返回异常值
		else
			return nullopt;
	}

	//原始属性注册
	void EffectComponent::property_slot_sign(unordered_map<std::string, double>& property_slot)
	{
		//注册原始属性
		original_property = &property_slot;
	}

	//行为决策脚本功能注册
	void EffectComponent::function_sign(LuaState& action_script)
	{
		//传入函数调用
		action_script.set_function("effect_script_sign", [this](const json& config) -> void
			{
				this->add_effect(config);
			});
		action_script.set_function("effect_work_get", [this](const std::string& effect_name) ->const EffectMap&
			{
				return this->effect_work_get(effect_name);
			});
		action_script.set_function("property_modifier_get", [this](const std::string& prop_key) -> optional<double>
			{
				return this->property_modifier_get(prop_key);
			});
	}
}

