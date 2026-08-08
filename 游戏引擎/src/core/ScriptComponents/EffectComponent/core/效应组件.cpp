#include "src/core/ScriptComponents/EffectComponent/局部命名空间使用.h"
#include "src/tools/Logging/日志系统.h"

//引擎命名空间
namespace engine
{
	//构造函数
	EffectComponent::EffectComponent()
	{
		//获取权限密钥
		acl_key = event_terminal.acl_key_gen();
	}

	//效应配置解析
	bool EffectComponent::effect_config_parse(unique_ptr<EffectSlot>& effect, const json& config)
	{
		//若读取路径字段无效
		if (!config_checker.field_check<string>(config,"effect_path"))
			return false;
		else
		{
			//获取读取路径
			string config_path = config["effect_path"];
			//若读取路径无效
			if (!config_checker.path_check(config_path))
				return false;
			else
			    effect->effect.load_file(config["effect_path"]);
		}

		//若名称字段无效
		if (!config_checker.field_check<string>(config, "effect_name"))
			return false;
		else
			effect->name = config["name"];

		//若叠加上限字段无效
		if (!config_checker.field_check<string>(config, "max_stacks"))
			return false;
		else
			effect->max_stacks = config["max_stacks"];

		//若执行优先级字段无效
		if (!config_checker.field_check<string>(config, "priority"))
			return false;
		else
			effect->priority = config["priority"];

		//若执行次序字段无效
		if (!config_checker.field_check<string>(config, "phase_mask"))
			return false;
		else
			effect->phase_mask = config["phase_mask"];

		//若所有字段均存在则返回true
		return true;
	}

	//效应添加
	bool EffectComponent::add_effect(const json& config)
	{
		//构造新效应
		unique_ptr<EffectSlot> ptr_new_effect(new(nothrow) EffectSlot());
		//若效应构造失败
		if (!ptr_new_effect)
			return false;
		//若效应配置解析异常
		if(!effect_config_parse(ptr_new_effect, config))
		    return false;
		//若效应配置解析正常
		else
		{
			//简化表示路径
			auto& new_effect = *ptr_new_effect;
			auto& effect_name = new_effect.name;
			
			//获取事件终端
			auto& terminal = new_effect.event_terminal;
			//构造事件入口
			auto event_entry = [this](std::vector<std::shared_ptr<config_event>> events)->void
				{
					this->event_process(events);
				};
			//配置事件终端
			terminal.event_entry_register(event_entry);

			//若新效应已存在
			if (effect_set.count(effect_name))
			{
				//简化表示路径
				auto& original_effect = *effect_set[effect_name];
				//若原效应未达到堆叠上限
				if (original_effect.max_stacks != original_effect.now_stacks)
					original_effect.now_stacks++;
			}
			//若新效应不存在
			else
				effect_set.insert({ effect_name,move(ptr_new_effect) });
		}

	}

	//效应移除
	bool EffectComponent::remove_effect(const string& effect_name)
	{
		//若脚本不存在则返回卸载失败
		if (!effect_set.count(effect_name))
			return false;

		//卸载待卸载脚本
		effect_set.erase(effect_name);
		//返回卸载成功
		return true;
	}

	//效应执行
	void EffectComponent::run_effects(EffectPhase phase)
	{
		for (auto& effect_slot : effect_set)
		{
			//简化表示路径
			auto& effect = effect_slot.second;
			//若执行阶段标记匹配
			if (effect.get()->phase_mask == phase)
				effect.get()->effect["action"]();
		}
	}

	//效应效果映射获取
	const vector<EffectMap>& EffectComponent::effect_map_get(const string& effect_name)
	{
		//若存在指定效应
		if (effect_map_set.count(effect_name))
			return effect_map_set[effect_name];
		//若不存在则返回空表
		else
			return {};
	}

	//修正属性获取
	optional<double> EffectComponent::property_modifier_get(const string& prop_key)
	{
		//若存在指定属性
		if (modifiers_property_cache.count(prop_key))
			return modifiers_property_cache[prop_key];
		//若不存在则返回异常值
		else
			return nullopt;
	}

	//原始属性注册
	void EffectComponent::property_slot_register(unordered_map<string, double>& property_slot)
	{
		//注册原始属性
		original_property = &property_slot;
	}

	//行为决策脚本功能注册
	void EffectComponent::function_register(LuaState& action_script)
	{
		//传入函数调用
		action_script.set_function("effect_script_register", [this](const json& config) -> void
			{
				this->add_effect(config);
			});
		action_script.set_function("effect_work_get", [this](const string& effect_name) ->const vector<EffectMap>&
			{
				return this->effect_map_get(effect_name);
			});
		action_script.set_function("property_modifier_get", [this](const string& prop_key) -> optional<double>
			{
				return this->property_modifier_get(prop_key);
			});
	}

	//事件处理
	void EffectComponent::event_process(std::vector<std::shared_ptr<config_event>> events)
	{

	}
}

