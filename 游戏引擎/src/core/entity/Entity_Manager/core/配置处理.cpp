#include "../局部命名空间使用.h"
#include "src/tools/Non_GUI/Logging/日志系统.h"

//引擎命名空间
namespace engine
{
    //构造函数
    Entity_Manager::Entity_Manager()
    {
        //生成权限密钥
        acl_key = event_terminal.acl_key_gen();
    }

    //属性槽管理器绑定
    void Entity_Manager::bind_property_manager(Property_Manager* prop_manager)
    {
        this->prop_manager = prop_manager;
    }

    //事件中转站接入
    void Entity_Manager::attach(void)
    {
        //订阅事件集合记录
        vector<config_event> needed_events(event_map.begin(), event_map.end());
        //构造事件接收入口
        auto event_receive_entry = [this](shared_ptr<config_event> evt)-> void
            {
                this->outer_event_process(evt);
            };
        //注册事件接收入口
        event_terminal.receive_entry_register(event_receive_entry);
        //更新接入信息
        event_terminal.attach("Entity_Manager", needed_events, acl_key);
    }

    //配置事件解析
    bool Entity_Manager::config_field_parse(const json& config)
    {
        //若实体类型字段无效
        if (!config_checker.field_check<string>(config, "type"))
            return false;
        //若决策树加载路径字段无效
        if (!config_checker.field_check<string>(config, "decision_load_path"))
            return false;
        //若权限列表字段无效
        if (!config_checker.field_check<vector<string>>(config, "acls"))
            return false;
        //若订阅事件列表字段无效
        if (!config_checker.field_check<vector<pair<string, string>>>(config, "needed_events"))
            return false;

        //若所有检查均通过
        return true;
    }

    //决策树加载路径注册
    void Entity_Manager::decision_tree_register(const string& entity_type, const string& path)
    {
        //若当前实体类型未注册决策树加载路径
        if (!decision_load_paths.count(entity_type))
            decision_load_paths.insert({ entity_type,path });
    }

    //实体从属权限注册
    void Entity_Manager::owner_acl_register(const string& master, const vector<string>& minion_set)
    {
        //拷贝权限数据
        acl_set.push_back({ master ,minion_set });
        //对新权限进行内部字典序排序
        sort(acl_set.back().minion_set, less());
        //对权限集合进行字典序排序
        sort(acl_set, less(), &minion_acl::master);
    }

}
