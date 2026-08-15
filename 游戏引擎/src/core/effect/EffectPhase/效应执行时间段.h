#pragma once
//预编译头
#include "common/前置头文件包含.h"

namespace engine
{
    //效应执行时间段
    enum EffectPhase : uint32_t
    {
        //空标记
        None = 0,
        //行为决策前
        PreBehavior = 1 << 0,
        //行为决策后
        PostBehavior = 1 << 1,
        //周期性执行（需引擎提供定时器支持）
        OnTick = 1 << 2,
    };

    // ------------------ 位运算符重载 ------------------
    inline EffectPhase operator|(EffectPhase a, EffectPhase b) 
    {
        return static_cast<EffectPhase>
            (static_cast<uint32_t>(a) |
             static_cast<uint32_t>(b));
    }

    inline EffectPhase operator&(EffectPhase a, EffectPhase b)
    {
        return static_cast<EffectPhase>
            (static_cast<uint32_t>(a) & 
             static_cast<uint32_t>(b));
    }

    inline EffectPhase operator^(EffectPhase a, EffectPhase b) 
    {
        return static_cast<EffectPhase>
            (static_cast<uint32_t>(a) ^ 
             static_cast<uint32_t>(b));
    }

    inline EffectPhase operator~(EffectPhase a) 
    {
        return static_cast<EffectPhase>(~static_cast<uint32_t>(a));
    }

    inline EffectPhase& operator|=(EffectPhase& a, EffectPhase b) 
    {
        a = a | b;
        return a;
    }

    inline EffectPhase& operator&=(EffectPhase& a, EffectPhase b)
    {
        a = a & b;
        return a;
    }

    inline EffectPhase& operator^=(EffectPhase& a, EffectPhase b) 
    {
        a = a ^ b;
        return a;
    }

    // ------------------ 辅助工具函数 ------------------

    // 检查是否包含某个阶段
    inline bool has_phase(EffectPhase value, EffectPhase phase)
    {
        //返回检查
        return (value & phase) == phase; 
    }

    //字符串转换
    inline std::string to_string(EffectPhase phase) 
    {
        //若为空标记则直接转化
        if (phase == EffectPhase::None)
            return "None";

        std::string result;
        //依次匹配枚举值
        if (has_phase(phase, EffectPhase::PreBehavior))  
            result += "PreBehavior|";
        if (has_phase(phase, EffectPhase::PostBehavior))
            result += "PostBehavior|";
        if (has_phase(phase, EffectPhase::OnTick))      
            result += "OnTick|";
        //若转化结果不为空则去除末尾'|'
        if (!result.empty()) 
            result.pop_back();
        return result;
    }

    //字符串枚举解析
    inline EffectPhase phase_from_string(const std::string& str) 
    {
        //静态变量避免反复初始化
        static const std::unordered_map<std::string, EffectPhase> map = 
        {
            {"PreBehavior",  EffectPhase::PreBehavior},
            {"PostBehavior", EffectPhase::PostBehavior},
            {"OnTick",       EffectPhase::OnTick}
        };
        //获取目标迭代器
        auto it = map.find(str);
        //若迭代器有效
        if (it != map.end()) 
            return it->second;
        //若迭代器无效
        else
            return EffectPhase::None; 
    }

    //字符串集合枚举解析
    inline EffectPhase phases_from_strings(const std::vector<std::string>& strs) 
    {
        EffectPhase result = EffectPhase::None;
        //循环解析
        for (const auto& str : strs)
            result |= phase_from_string(str);
        //返回结果
        return result;
    }
}