#include "src/core/ScriptComponents/SkillWrapComponent/局部命名空间使用.h"

namespace engine
{
    //脚本添加
    void SkillWrapComponent::add_script(const std::string& script_name, const std::string& script_path)
    {
        //若键值对不存在则插入新脚本
        if (!script_set.count(script_name))
            script_set.insert({ script_name,make_unique<LuaState>() });
        //创建脚本
        script_set[script_name].reset(new(nothrow) LuaState());
        //若创建成功则加载脚本内容
        if (script_set[script_name])
            (*script_set[script_name]).load_file(script_path);
    }

    //脚本移除
    void SkillWrapComponent::remove_script(const std::string& script_name)
    {
        //若存在指定名称脚本
        if (script_set.count(script_name))
            script_set.erase(script_name);
        else
            return;
    }

    //脚本执行
    sol::table SkillWrapComponent::recall_script(const std::string& script_name)
    {
        //若存在指定名称脚本
        if (script_set.count(script_name))
            //返回打包结果
            return (*(script_set[script_name].get()))["wrap"](sol::as_table(*ptr_property));
        else
            return {};
    }

    //通用属性槽注册
    void SkillWrapComponent::property_slot_register(const unordered_map<std::string, double>& property_slot)
    {
        //接收数据
        ptr_property = &property_slot;
    }

    //行为决策脚本功能注册
    void SkillWrapComponent::function_register(LuaState& action_script)
    {
        action_script.set_function("wrap_script_register", [this](const std::string& name, const std::string& path)->void
            {
                this->add_script(name, path);
            });
        action_script.set_function("wrap_script_unload", [this](const std::string& name) -> void
            {
                this->remove_script(name);
            });
        action_script.set_function("wrap_script_call", [this](const std::string& name) -> sol::table
            {
                return this->recall_script(name);
            });

    }

}
