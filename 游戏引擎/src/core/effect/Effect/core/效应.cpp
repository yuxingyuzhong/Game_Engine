#include "../局部命名空间使用.h"
#include "src/tools/Non_GUI/Logging/日志系统.h"

namespace engine
{
    //构造函数
    Prop_Effect::Prop_Effect()
    {
        //获取权限密钥
        acl_key = event_terminal.acl_key_gen();
    }

    //析构函数
    Prop_Effect::~Prop_Effect()
    {

    }

    //配置读取
    bool Prop_Effect::config_read(Config_Checker& config_checker, const json& config)
    {
        //若读取路径字段无效
        if (!config_checker.field_check<string>(config, "path"))
            return false;
        else
        {
            //获取读取路径
            path config_path = Engine_Env::absolute_path_get(config["path"].get<string>());
            //若读取路径无效
            if (!config_checker.path_check(config_path))
                return false;
            //若读取路径有效
            else
            {
                //重置状态机
                script = LuaState{};
                //加载新状态机
                script.load_file(path_to_string(config_path));
                //打开所有标准库
                script.open_libraries();
            }
        }

        //若归属字段无效
        if (!config_checker.field_check<uint64_t>(config, "inclusion"))
            return false;
        //若名称字段无效
        if (!config_checker.field_check<string>(config, "name"))
            return false;

        //获取效应归属
        inclusion = config["inclusion"].get<uint64_t>();
        //获取效应名称
        name = config["name"].get<string>();

        //若所有字段均存在则返回true
        return true;
    }

    //数据注入
    void Prop_Effect::data_injection()
    {
        //若效应未绑定ID
        if (!ID.has_value())
        {
            Log::error("Effect::效应ID未绑定\n无法完成数据注入");
            return;
        }
        //注册效应归属
        script.set("effect_inclusion", inclusion);
        //注册效应ID
        script.set("effect_ID", ID.value());
        //注册效应名称
        script.set("effect_name",name);
        //注册效应作用对象
        script.set("effect_object", sol::as_table(*effect_object));

        //注册事件基类信息
        register_event(script);
        //注册配置事件信息
        register_config_event(script);
        //注册事件集合引用
        script.set("event_set", ref(event_terminal.event_get(acl_key)));

        //注册事件发送函数
        script.set_function("event_send", [this](shared_ptr<config_event> event)->void
            {
                this->event_terminal.event_send({ event }, acl_key);
            });
    }

    //ID绑定
    void Prop_Effect::ID_bind(const uint64_t& ID)
    {
        //若ID未绑定则绑定ID
        if(!this->ID.has_value())
            this->ID = ID;
    }

    //作用对象绑定
    void Prop_Effect::effect_object_bind(std::unordered_map<std::string, double>* object)
    {
        effect_object = object;
    }

    //效应名称获取
    const std::string& Prop_Effect::effect_name_get(void)
    {
        return name;
    }

    //效应触发
    void Prop_Effect::effect_act(void)
    {
        script["action"]();
    }

}
