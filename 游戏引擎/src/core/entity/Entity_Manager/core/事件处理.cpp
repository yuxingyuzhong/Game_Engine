#include "../局部命名空间使用.h"
#include "src/tools/Non_GUI/Logging/日志系统.h"

//引擎命名空间
namespace engine
{
    //事件广播
    void Entity_Manager::event_broadcast(shared_ptr<config_event> event)
    {
        //向所有实体发送事件
        for (auto& entity_record : entity_set)
            entity_record.entity.event_terminal.event_receive(event);
    }

    //事件定向发送
    bool Entity_Manager::event_unicast(const std::string& type, const uint64_t& ID,
        std::shared_ptr<config_event> event)
    {
        //获取指定实体索引
        int64_t target_index = entity_record_seek(ID);
        //若实体索引无效
        if (target_index < 0)
            return false;

        //获取目标实体
        auto& entity = entity_set[target_index].entity;
        //若实体类型与实际类型不匹配
        if (entity.type_get() != type)
            return false;

        //向指定书体发送事件
        entity_set[target_index].entity.event_terminal.event_receive(event);
        //返回发送成功
        return true;
    }

    //外部事件处理
    void Entity_Manager::outer_event_process(shared_ptr<config_event> event)
    {
        //若当前为配置事件
        if (event->category == "Config")
        {
            //简化表示路径
            auto& config = event->config;

            //若配置字段检查通过
            if (config_field_parse(config))
            {
                //获取实体决策树加载路径
                path decision_load_path = Engine_Env::absolute_path_get(config["decision_load_path"].get<string>());
                //记录实体决策树加载路径
                decision_tree_register(config["type"], path_to_string(decision_load_path));
                //记录实体类型从属权限
                owner_acl_register(config["type"], config["acls"]);

                //缓冲解析结果
                vector<pair<string, string>> buffer = config["needed_events"];
                //转化解析结果
                for (int transform_time = 0; transform_time < buffer.size(); transform_time++)
                {
                    //简化表示路径
                    auto& tag = buffer[transform_time];
                    //构造事件
                    config_event needed_event("", tag.first, tag.second, json::object());
                    //若该事件不存在
                    if (!event_map.count(needed_event))
                        event_map.insert(needed_event);
                }

                //更新接入信息
                attach();
            }
        }
        //若当前非配置事件
        else
        {
            //简化表示路径
            auto& tag = event->tag;
            auto& config = event->config;

            //若实体类型字段无效
            if (!config_checker.field_check<string>(config, "target_type"))
                return;
            //获取目标实体类型
            string target_type = config["target_type"];

            //若为创建分支事件
            if (tag == "Build")
            {
                //新建实体ID集合
                vector<uint64_t> ID_set{};
                //创建指定数量实体
                ID_set = entity_build(target_type, config.value<int>("counts", 0));

                //若ID集合为空则直接返回
                if (!ID_set.empty())
                {
                    Log::warn("Entity_Manager::未定义实体创建数量\n实体创建事件已驳回");
                    return;
                }
                else
                {
                    //删除"counts"字段
                    config.erase("counts");
                    //增加"ID_set"字段
                    config.emplace("ID_set", ID_set);
                    //广播事件
                    event_broadcast(event);
                    //向外界发布修饰后事件
                    event_terminal.event_send(event, acl_key);
                }
            }
            else if (tag == "Unload")
            {
                //若待卸载实体ID集合字段无效
                if (!config_checker.field_check<vector<int64_t>>(config, "ID_set"))
                    return;
                //获取待卸载实体ID
                vector<uint64_t> ID_set = config["ID_set"];

                //卸载合法操作实体
                entity_unload(ID_set);
                //广播事件
                event_broadcast(event);
            }
            //若为其他事件
            else
            {
                //若目标实体ID字段无效
                if (!config_checker.field_check<string>(config, "target_ID"))
                    return;
                //获取目标实体ID
                uint64_t target_ID = config["target_ID"];

                //定向发送事件
                event_unicast(target_type, target_ID, event);
            }
        }
    }

    //内部事件仲裁
    void Entity_Manager::inner_event_govern(shared_ptr<config_event> event)
    {
        //简化表示路径
        auto& category = event->category;
        auto& tag = event->tag;
        auto& config = event->config;
        //获取事件发起者类型
        string sender_type = config["sender_type"];
        //获取事件发起者ID
        uint64_t sender_ID = config["sender_ID"];

        //若事件分类为实体
        if (category == "Entity")
        {
            //若为创建分支事件
            if (tag == "Build")
            {
                //若目标实体类型字段无效
                if (!config_checker.field_check<string>(config, "target_type"))
                {
                    Log::warn("Entity_Manager::未定义目标实体类型\n实体创建事件已驳回");
                    return;
                }

                //获取目标实体类型
                string target_type = config["target_type"];

                //获取权限集合查找索引
                int acl_index = acl_record_seek(sender_type);
                //若返回索引无效
                if (acl_index < 0)
                {
                    Log::warn("Entity_Manager::实体未注册从属权限\n从属实体构建事件已驳回");
                    return;
                }
                //若返回索引有效
                else
                {
                    //简化表示路径
                    auto& acl = acl_set[acl_index];
                    //检查操作是否具有合法权限
                    for (int match_time = 0; match_time < acl.minion_set.size(); match_time++)
                    {
                        //若成功匹配权限
                        if (target_type == acl.minion_set[match_time])
                        {
                            //若实体创建数目字段无效
                            if (!config_checker.field_check<vector<uint64_t>>(config, "counts"))
                            {
                                Log::warn("Entity_Manager::从属创建未定义创建数量\n事件已驳回");
                                return;
                            }

                            //新建实体ID记录
                            vector<uint64_t> ID_set{};
                            //创建指定数量实体
                            ID_set = entity_build(target_type, config["counts"].get<uint64_t>(), sender_ID);
                            //删除"counts"字段
                            config.erase("counts");
                            //增加"ID_set"字段(构造回复信息)
                            config.emplace("ID_set", ID_set);
                            //广播事件(包含对事件发送者的回复)
                            event_broadcast(event);
                        }
                    }
                }
            }
            //若为卸载分支事件
            else if (tag == "Unload")
            {
                //若待卸载实体ID集合字段无效
                if (!config_checker.field_check<vector<uint64_t>>(config, "ID_set"))
                {
                    Log::warn("Entity_Manager::未定义目标实体ID\n实体卸载事件已驳回");
                    return;
                }

                //获取待卸载实体ID
                vector<uint64_t> ID_set = config["ID_set"];

                //卸载目标实体从属记录
                vector<uint64_t> valid_IDs = minion_record_erase(sender_ID, ID_set);
                //若存在非从属实体卸载
                if (valid_IDs.size() != ID_set.size())
                    Log::warn("Entity_Manager::从属卸载请求存在非从属实体");

                //卸载合法从属实体
                entity_unload(valid_IDs);
                //重置事件信息
                config["ID_set"] = valid_IDs;

                //广播事件(包含对事件发送者的回复)
                event_broadcast(event);
            }
            //若为转移分支事件
            else if (tag == "Transfer")
            {
                //若从属接收实体类型字段无效
                if (!config_checker.field_check<string>(config, "target_type"))
                {
                    Log::warn("目标实体类型未定义\n从属转移事件已驳回");
                    return;
                }
                //若待卸载实体ID集合字段无效
                if (!config_checker.field_check<uint64_t>(config, "target_ID"))
                {
                    Log::warn("目标实体ID未定义\n从属转移事件已驳回");
                    return;
                }
                //若待卸载实体ID集合字段无效
                if (!config_checker.field_check<vector<uint64_t>>(config, "ID_set"))
                {
                    Log::warn("转移实体ID 集合未定义\n从属转移事件已驳回");
                    return;
                }

                //目标实体类型记录
                string target_type = config["target_type"].get<string>();
                //目标实体ID记录
                uint64_t target_ID = config["target_ID"].get<uint64_t>();
                //转移实体ID集合记录
                vector<uint64_t> ID_set = config["ID_set"].get<vector<uint64_t>>();

                //发起者删除从属实体记录
                vector<uint64_t> valid_IDs = minion_record_erase(sender_ID, ID_set);
                //若存在非法从属转移
                if (valid_IDs.size() < ID_set.size())
                    Log::warn("Entity_Manager::存在非法从属转移\n目标从属不属于事件发起者");
                //接收者添加从属实体记录
                minion_record_add(target_ID, valid_IDs);

            }
            //若为请求/命令分支事件
            else if (tag == "Request" || tag == "Command")
            {
                //若目标实体类型字段无效
                if (!config_checker.field_check<string>(config, "target_type"))
                {
                    Log::warn("Entity_Manager::未定义目标实体类型\n实体请求/命令事件已驳回");
                    return;
                }
                //若目标实体ID字段字段无效
                if (!config_checker.field_check<vector<int64_t>>(config, "target_ID"))
                {
                    Log::warn("Entity_Manager::未定义目标实体ID\n实体请求/命令事件已驳回");
                    return;
                }

                //获取目标实体类型
                string target_type = config["target_type"];
                //获取目标实体ID
                uint64_t target_ID = config["target_ID"].get<uint64_t>();

                //获取目标实体索引
                int64_t entity_index = entity_record_seek(target_ID);
                //若索引无效
                if (entity_index < 0)
                {
                    Log::warn("Entity_Manager::目标实体不存在\n实体请求/命令事件已驳回");
                    return;
                }

                //简化表示路径
                auto& target_entity = entity_set[entity_index].entity;
                //若目标实体类型错误
                if (target_entity.type_get() != target_type)
                {
                    Log::warn("Entity_Manager::目标实体类型错误\n实体请求/命令事件已驳回");
                    return;
                }

                //若为命令分支则进一步检测
                if (tag == "Command")
                {
                    //从属记录读取索引
                    int minion_index = minion_record_seek(sender_ID);
                    //若返回索引无效
                    if (minion_index < 0)
                    {
                        Log::warn("Entity_Manager::从属记录不存在\n从属实体命令事件非法");
                        return;
                    }
                    else
                    {
                        //简化表示路径
                        auto& minion_IDs = minion_records[minion_index].minion_IDs;
                        //匹配从属实体身份
                        for (auto& minion_ID : minion_IDs)
                        {
                            //若目标实体匹配成功
                            if (minion_ID == target_ID)
                                //发送事件
                                target_entity.event_terminal.event_receive(event);
                        }
                    }
                }
                else
                    //发送事件
                    target_entity.event_terminal.event_receive(event);
            }
        }

        //管理器内部处理完毕后发送事件
        event_terminal.event_send({ event }, acl_key);
    }

}
