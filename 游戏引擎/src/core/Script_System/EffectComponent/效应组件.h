#pragma once
#include "common/前置头文件包含.h"
#include "src/core/event/Event_Broker/事件中转站通信结构体.h"

//脚本系统模块
namespace engine
{
    //效应槽
	struct EffectSlot 
    {
    private:
        //脚本别名
        using LuaState = sol::state;
    public:
        //效应名称
        std::string name{};
        //效应分组
        std::string group{};
        //叠加上限
        uint64_t max_stacks = 1;
        //同类效应链表
        std::list<EffectSlot*> others{};

        //Lua脚本实例
		LuaState script;    
        //执行优先级（数值越小越先执行）
		double priority = 0;              
        //执行次序标记
		uint32_t phase_mask = 0;       
    };

    //效应效果映射
    struct EffectMap
    {
        //目标属性
        std::string target{};
        //效果倍率
        double effect = 1.00;
    };

    //效应触发时间段
	enum EffectPhase : uint32_t 
    {
        //行为决策前
		PreBehavior = 1 << 0, 
        //行为决策后
		PostBehavior = 1 << 1,  
        //周期性触发（需引擎提供定时器支持）
		OnTick = 1 << 2,  
	};

    //效应组件
    class EffectComponent 
    {
    private:
        //脚本别名
        using LuaState = sol::state;
    public:
        // --- 效应管理接口 ---

        //效应添加
        void add_effect(const nlohmann::json& config);

    private:
        //效应移除
        void remove_effect(const uint64_t& index);
    public:
        // --- 执行调度接口 ---

        //按阶段执行所有匹配的效应
        void run_effects(EffectPhase phase); 

        // --- 查询接口 ---

        //效应效果获取
        const EffectMap& effect_work_get(const std::string& effect_name);
        //修正属性获取
        std::optional<double> property_modifier_get(const std::string& prop_key);

        // --- 交互接口 ---

        //原始属性注册
        void property_slot_sign(std::unordered_map<std::string, double>& property_slot);

        //行为决策脚本功能注册
        void function_sign(LuaState& action_script);

    private:
        //效应集合
        std::vector<std::unique_ptr<EffectSlot>> effects;      
        //效应效果映射集合
        std::unordered_map<std::string, EffectMap> effect_map_set{};
        //修正属性缓冲
        std::unordered_map<std::string, double> modifiers_property_cache;
        //原始属性
        std::unordered_map<std::string, double>* original_property;

        //字段初始化
        bool field_initialize(std::unique_ptr<EffectSlot>& effect,const nlohmann::json& config);

        //同类效应更新
        void homogeneous_effect_update(const uint64_t& leader_index, const uint64_t& new_index,
        bool replace);
    };
}
