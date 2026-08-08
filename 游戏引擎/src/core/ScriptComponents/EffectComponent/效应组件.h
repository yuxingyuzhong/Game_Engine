#pragma once
#include "common/前置头文件包含.h"
#include "common/types/事件类型.h"
#include "src/core/event/Event_Terminal/事件终端.h"
#include "common/external/Sol2/sol类型别名.h"
#include "src/tools/Config_Checker/配置检查器.h"

//脚本系统模块
namespace engine
{
    //效应槽
	struct EffectSlot 
    {
    public:
        //效应名称
        std::string name{};
        //叠加上限
        uint64_t max_stacks = 1;
        //叠加层数
        uint64_t now_stacks = 1;

        //Lua脚本实例
		LuaState effect;    
        //执行优先级（数值越小越先执行）
		double priority = 0;              
        //执行次序标记
		uint32_t phase_mask = 0;      

        //构造函数
        EffectSlot()
        {
            //获取权限密钥
            acl_key = event_terminal.acl_key_gen();
        }
    public:
        //事件终端
        Event_Terminal event_terminal;
    private:
        //事件发送密钥
        int64_t acl_key = 0;
    };

    //效应效果映射
    struct EffectMap
    {
        //目标属性
        std::string attribute{};
        //倍率加成
        double multiplier = 1.00;
        //直效加成
        double value = 0;
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
    public:
        //构造函数
        EffectComponent();
        //析构函数
        ~EffectComponent() = default;
        
        // ———— 效应管理 ————

        //效应添加
        bool add_effect(const nlohmann::json& config);
        //效应移除
        bool remove_effect(const std::string& effect_name);
        //效应执行
        void run_effects(EffectPhase phase); 

        // ———— 效果查询接口 ————

        //效应效果映射获取
        const std::vector<EffectMap>& effect_map_get(const std::string& effect_name);
        //修正属性获取
        std::optional<double> property_modifier_get(const std::string& prop_key);

        // ———— 交互 ————

        //原始属性注册
        void property_slot_register(std::unordered_map<std::string, double>& property_slot);

        //行为决策脚本功能注册
        void function_register(LuaState& action_script);

    public:
        //事件终端
        Event_Terminal event_terminal;
    private:
        //事件发送密钥
        int64_t acl_key = 0;

        //效应集合
        std::unordered_map<std::string,std::unique_ptr<EffectSlot>> effect_set;
        //效应效果映射集合
        std::unordered_map<std::string, std::vector<EffectMap>> effect_map_set{};
        //修正属性缓冲
        std::unordered_map<std::string, double> modifiers_property_cache;
        //原始属性
        std::unordered_map<std::string, double>* original_property;

        //效应配置检查器
        Config_Checker config_checker;
        //效应配置解析
        bool effect_config_parse(std::unique_ptr<EffectSlot>& effect,const nlohmann::json& config);

        //事件处理
        void event_process(std::vector<std::shared_ptr<config_event>> events);

    };
}
