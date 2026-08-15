#pragma once
//预编译头
#include "common/前置头文件包含.h"
//获取预定义事件类型
#include "common/types/事件类型.h"
//获取事件终端
#include "src/core/event/Event_Terminal/事件终端.h"
//获取效应执行时间段
#include "src/core/effect/EffectPhase/效应执行时间段.h"
//获取预定义sol2库类型别名
#include "common/external/Sol2/sol类型别名.h"
//获取配置检查器
#include "src/tools/Non_GUI/Config_Checker/配置检查器.h"

//脚本系统模块
namespace engine
{
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

    //效应
    class Prop_Effect
    {
    private:
        //效应归属
        uint64_t inclusion{};
        //效应编号
        uint64_t ID = -1;
        //效应名称
        std::string name{};

        //叠加上限
        uint64_t max_stacks = 1;
        //叠加层数
        uint64_t now_stacks = 1;

        //Lua脚本实例
        LuaState script;

        //抗消除级别(数值越大抗消除能力越强)
        std::optional<uint64_t> undispel_levels = 0;
        //消除级别(数值越大消除能力越强)
        uint64_t dispel_levels = 0;

        //执行优先级(数值越大越先执行)
        std::optional<uint64_t> priority = 0;
        //执行次序标记
        uint32_t phase_mask = 0;

        //效应作用对象
        std::unordered_map<std::string, double>* effect_object;
    public:
        //事件终端
        Event_Terminal event_terminal;
    private:
        //事件发送密钥
        int64_t acl_key = 0;
    public:
        //构造函数
        Prop_Effect();
        //析构函数
        ~Prop_Effect();
        //默认移动构造
        Prop_Effect(Prop_Effect&&) = default;   
        //默认移动赋值
        Prop_Effect& operator=(Prop_Effect&&) = default; 

        //配置读取
        bool config_read(Config_Checker& config_checker, const nlohmann::json& config);
        //数据注入
        void data_injection(void);
        //编号绑定
        void ID_bind(const uint64_t& ID);
        //作用对象绑定
        void effect_object_bind(std::unordered_map<std::string, double>* object);

        //效应归属获取
        const uint64_t& effect_inclusion_get(void);
        //效应编号获取
        const uint64_t& effect_ID_get(void);
        //效应名称获取
        const std::string& effect_name_get(void);

        //当前堆叠层数获取
        uint64_t now_stacks_get(void) const;
        //最大堆叠层数获取
        uint64_t max_stacks_get(void) const;
        //堆叠层数修改
        int64_t now_stacks_amend(const int64_t& amend_counts);

        //执行阶段获取
        std::optional<uint64_t> priority_get(void);
        //执行优先级获取
        uint32_t phase_mask_get(void);

        //抗消除级别获取
        std::optional<uint64_t> undispel_levels_get(void);
        //消除级别获取
        uint64_t dispel_levels_get(void);

        //效应触发
        void effect_act(void);
    };
}
