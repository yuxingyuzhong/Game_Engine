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
            string config_path = config["path"];
            //若读取路径无效
            if (!config_checker.path_check(config_path))
                return false;
            //若读取路径有效
            else
            {
                //重置状态机
                script = LuaState{};
                //加载新状态机
                script.load_file(config["path"]);
                //打开所有标准库
                script.open_libraries();
            }
        }

        //若归属字段无效
        if (!config_checker.field_check<uint64_t>(config, "inclusion"))
            return false;
        else
            inclusion = config["inclusion"].get<uint64_t>();

        //若名称字段无效
        if (!config_checker.field_check<string>(config, "name"))
            return false;
        else
            name = config["name"].get<string>();

        //若叠加上限字段无效
        if (!config_checker.field_check<uint64_t>(config, "max_stacks"))
            return false;
        else
            max_stacks = config["max_stacks"].get<uint64_t>();

        //若抗消除级别字段未定义
        if (!config.contains("undispel_levels"))
        {
            Log::warn("Prop_Effect::未定义抗消除级别字段\n效应无法加载");
            return false;
        }
        //若抗消除字段仅可转化为字符串
        else if (!config.is_number_integer() && config.is_string())
            undispel_levels = nullopt;
        //若抗消除字段可转化为整数类型
        else if(config.is_number_integer())
            undispel_levels = config["undispel_levels"].get<uint64_t>();

        //若消除级别字段无效
        if (!config_checker.field_check<uint64_t>(config, "dispel_levels"))
            return false;
        else
            dispel_levels = config["dispel_levels"].get<uint64_t>();

        //若执行优先级字段未定义
        if (!config.contains("priority"))
        {
            Log::warn("Prop_Effect::未定义执行优先级字段\n效应无法加载");
            return false;
        }
        //若执行优先级仅可转化为字符串
        else if (!config.is_number_integer() && config.is_string())
            priority = nullopt;
        //若执行优先级可转化为整数类型
        else if (config.is_number_integer())
            priority = config["priority"].get<uint64_t>();

        //若执行次序字段无效
        if (!config_checker.field_check<vector<string>>(config, "phase_mask"))
            return false;
        else
        {
            //缓冲执行次序字段内容
            vector<string> buffer = config["phase_mask"].get<vector<string>>();
            //转化为为执行次序
            phase_mask = phases_from_strings(buffer);
        }

        //若所有字段均存在则返回true
        return true;
    }

    //数据注入
    void Prop_Effect::data_injection()
    {

    }

    //ID绑定
    void Prop_Effect::ID_bind(const uint64_t& ID)
    {
        //若ID尚未设置
        if(ID == -1)
           this->ID = ID;
    }

    //作用对象绑定
    void Prop_Effect::effect_object_bind(std::unordered_map<std::string, double>* object)
    {
        effect_object = object;
    }

    //效应归属获取
    const uint64_t& Prop_Effect::effect_inclusion_get(void)
    {
        return inclusion;
    }

    //效应编号获取
    const uint64_t& Prop_Effect::effect_ID_get(void)
    {
        return ID;
    }

    //效应名称获取
    const std::string& Prop_Effect::effect_name_get(void)
    {
        return name;
    }

    //堆叠层数获取
    uint64_t Prop_Effect::now_stacks_get(void) const
    {
        return now_stacks;
    }

    //最大堆叠层数获取
    uint64_t Prop_Effect::max_stacks_get(void) const
    {
        return max_stacks;
    }

    //堆叠层数修改
    int64_t Prop_Effect::now_stacks_amend(const int64_t& amend_counts)
    {
        //缓存当前堆叠层数
        int64_t buffer = now_stacks;
        //修改当前堆叠层数
        now_stacks += amend_counts;
        //若堆叠层数小于0
        if (now_stacks < 0)
            //重置当前堆叠层数
            now_stacks = 0;
        //若堆叠层数大于最大堆叠层数
        else if (now_stacks > max_stacks)
            //重置当前堆叠层数
            now_stacks = max_stacks;

        //返回有效修改层数
        return now_stacks - buffer;
    }

    //执行阶段获取
    std::optional<uint64_t> Prop_Effect::priority_get(void) const
    {
        return priority;
    }

    //执行优先级获取
    uint32_t Prop_Effect::phase_mask_get(void) const
    {
        return phase_mask;
    }

    //抗消除级别获取
    std::optional<uint64_t> Prop_Effect::undispel_levels_get(void)
    {
        return undispel_levels;
    }

    //消除级别获取
    uint64_t Prop_Effect::dispel_levels_get(void)
    {
        return dispel_levels;
    }

    //效应触发
    void Prop_Effect::effect_act(void)
    {
        script["action"]();
    }

}
