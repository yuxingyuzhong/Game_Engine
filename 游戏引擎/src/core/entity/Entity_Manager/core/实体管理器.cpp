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

    // ———— 配置相关 ————

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

    // ———— 实体相关 ———— 

     //自定义ID分布检测
    bool Entity_Manager::custom_ID_filter(const std::vector<uint64_t>& ID_set)
    {
        //检查指定ID分布
        for (auto ID : ID_set)
        {
            //若存在指定区间外ID
            if (ID >= start_ID)
            {
                Log::info("Entity_Manager::存在自定义区间外ID\n实体构建请求已驳回");
                return false;
            }
        }

        return true;
    }

    //权限记录查找
    int64_t Entity_Manager::acl_record_seek(const std::string& master)
    {
        return binary_search(acl_set, master, less(), &minion_acl::master);
    }

    //从属记录查找
    int64_t Entity_Manager::minion_record_seek(const uint64_t& ID)
    {
        return binary_search(minion_records,ID,less(), &minion_record::master_ID);
    }

    //实体记录查找
    int64_t Entity_Manager::entity_record_seek(const uint64_t& ID)
    {
        return binary_search(entity_set, ID, less(), &entity_record::ID);
    }

    //实体创建 —— 创建数目重载
    std::vector<uint64_t> Entity_Manager::entity_build(const std::string& type, const int& counts,
        std::optional<uint64_t> master_ID)
    {
        //实体ID记录
        vector<uint64_t> IDs{};
        //记录实体ID
        for (int record_times = 0; record_times < counts; record_times++)
            IDs.push_back(now_ID++);     
        //调用ID集合重载
        entity_build(type, IDs,master_ID);
        //返回实体ID集合
        return IDs;
    }

    //实体创建 —— ID集合重载
    void Entity_Manager::entity_build(const std::string& type, const std::vector<uint64_t>& IDs,
        std::optional<uint64_t> master_ID)
    {
        //若已存储对应类型的决策树加载路径且属性槽管理器已绑定
        if (decision_load_paths.count(type) && prop_manager)
        {
            //简化表示路径
            auto& load_path = decision_load_paths[type];

            //创建对应对象
            for (int build_time = 0; build_time < IDs.size(); build_time++)
            {
                //构造新实体记录
                entity_set.push_back({ master_ID,IDs[build_time] ,{} });
                //获取新实体
                auto& new_entity = entity_set.back().entity;
                     
                //构造待注入依赖
                auto event_entry = [this](shared_ptr<config_event> event)->void
                    {
                        //处理事件
                        this->inner_event_govern(event);
                    };
                //设置事件发送入口
                new_entity.event_terminal.send_entry_register(event_entry);
                //创建属性槽
                prop_manager->prop_slot_build(type, IDs[build_time]);
                //绑定属性槽
                new_entity.prop_slot_bind(prop_manager->prop_slot_get(IDs[build_time]));
                //检查从属权限
                for (auto& acl:acl_set)
                {
                    //若成功匹配则启用从属功能
                    if (acl.master == type)
                        new_entity.minion_function_enable();
                }   
                //加载决策树
                new_entity.decision_tree_load(load_path);
            }

            //若上级实体ID不为空
            if (master_ID.has_value())
            {
                //获取从属记录索引
                int64_t minion_index = minion_record_seek(master_ID.value());
                //若索引无效
                if (minion_index < 0)
                {
                    //创建从属记录
                    minion_records.push_back({ master_ID.value(),{}});
                    //获取记录索引
                    minion_index = minion_records.size() - 1;
                }
                //记录从属实体信息
                for (const auto& ID: IDs)
                    minion_records[minion_index].minion_IDs.push_back(ID);
            }

            //将新建实体升序排序
            sort(entity_set, less(), &entity_record::ID);
        }
    }

    //实体卸载
    void Entity_Manager::entity_unload(vector<uint64_t>& IDs)
    {
        //降序排序防止迭代器失效
        sort(IDs, greater());

        for (int unload_index = 0; unload_index < IDs.size(); unload_index++)
        {
            //获取实体索引
            int64_t entity_index = entity_record_seek(IDs[unload_index]);
            //若实体查找得到有效索引
            if (entity_index >= 0)
            {
                //简化表示路径
                auto& entity = entity_set[entity_index].entity;
                auto& master_ID = entity_set[entity_index].master_ID;
                //销毁属性槽
                prop_manager->prop_slot_build(entity.type_get(), entity.ID_get());
                //若存在上级实体
                if (master_ID.has_value())
                {
                    //获取从属记录索引
                    int64_t minion_index = minion_record_seek(master_ID.value());
                    //若返回索引无效
                    if (minion_index < 0)
                        continue;
                    //简化表示路径
                    auto& minion_IDs = minion_records[minion_index].minion_IDs;
                    //匹配当前实体ID
                    for (int match_index = 0; match_index < minion_IDs.size();match_index++)
                    {
                        //若ID匹配成功
                        if (minion_IDs[match_index] == IDs[unload_index])
                        {
                            //卸载该ID记录
                            minion_IDs.erase(minion_IDs.begin() + match_index);
                            //结束匹配
                            break;
                        }
                    }
                }
                //获取从属记录索引
                int64_t minion_index = minion_record_seek(IDs[unload_index]);
                //若返回索引有效
                if (minion_index >= 0)
                {
                    //获取从属记录
                    auto& minion_IDs = minion_records[minion_index].minion_IDs;
                    //卸载从属实体上级实体标记
                    for (auto& minion_ID: minion_IDs)
                    {
                        //获取从属实体记录索引
                        int64_t entity_index = entity_record_seek(minion_ID);
                        //若返回索引无效
                        if (entity_index < 0)
                            continue;
                        //卸载上级实体标记
                        entity_set[entity_index].master_ID = nullopt;
                    }
                    //卸载从属记录
                    minion_records.erase(minion_records.begin() + minion_index);
                }
                //卸载目标实体
                entity_set.erase(entity_set.begin() + entity_index);
            }
        }
    }

    //实体行动
    void Entity_Manager::entity_act(void)
    {
        //待卸载实体ID集合
        vector<uint64_t> IDs{};
        //遍历所有实体
        for (int act_index = 0; act_index < entity_set.size(); act_index++)
        {
            //简化表示路径
            auto& entity = entity_set[act_index].entity;
            //若实体存活
            if (entity.is_alive())
                entity.act();
            //若实体死亡
            else
                IDs.push_back(entity.ID_get());
        }

        //卸载目标实体
        entity_unload(IDs);
    }

    // ———— 事件相关 ————

     //事件广播
    void Entity_Manager::event_broadcast(shared_ptr<config_event> event)
    {
        //向所有实体发送事件
        for (auto& entity_record: entity_set)
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
                //记录实体决策树加载路径
                decision_tree_register(config["type"], config["decision_load_path"]);
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

                //若为指定数量创建
                if(!ID_set.empty())
                {
                    //删除"counts"字段
                    config.erase("counts");
                    //增加"ID_set"字段
                    config.emplace("ID_set", ID_set);
                }
                //若为指定ID集合创建
                else
                {
                    //检验ID集合字段
                    if (!config_checker.field_check<vector<uint64_t>>(config, "ID_set"))
                    {
                        Log::warn("Entity_Manager::未匹配到实体创建所需数据\n创建失败");
                        return;
                    }
                    //获取指定ID集合
                    ID_set = config["ID_set"].get<vector<uint64_t>>();
                    //检查指定ID分布
                    custom_ID_filter(ID_set);
                    //创建指定ID实体
                    entity_build(target_type,ID_set);
                }

                //广播事件
                event_broadcast(event);
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
        string sender_type = config["sender_type"];
        uint64_t sender_ID = config["sender_ID"];

        //若目标实体类型字段无效
        if (!config_checker.field_check<string>(config, "target_type"))
            return;
        //获取目标实体类型
        string target_type = config["target_type"];

        //若事件分类为实体
        if (category == "Entity")
        {
            //若为创建分支事件
            if (tag == "Build")
            {
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
                            //新建实体ID集合
                            vector<uint64_t> ID_set{};
                            //创建指定数量实体
                            ID_set = entity_build(target_type, config.value<int>("counts", 0), sender_ID);
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
                    return;
                //获取待卸载实体ID
                vector<uint64_t> ID_set = config["ID_set"];

                //从属记录读取索引
                int minion_index = minion_record_seek(sender_ID);
                //若返回索引无效
                if (minion_index < 0)
                {
                    Log::warn("Entity_Manager::从属记录不存在\n从属实体卸载事件非法");
                    return;
                }

                //简化表示路径
                auto& minion_IDs = minion_records[minion_index].minion_IDs;
                //可卸载实体ID记录
                vector<uint64_t> valid_IDs{};
                //检查卸载操作是否合法
                for (int check_index = 0; check_index < ID_set.size(); check_index++)
                {
                    for (int match_index = 0; match_index < ID_set.size(); match_index++)
                    {
                        //简化表示路径
                        auto& minion_ID = minion_IDs[match_index];
                        //若ID匹配成功
                        if (ID_set[check_index] == minion_ID)
                            valid_IDs.push_back(minion_ID);
                    }
                }

                //卸载合法操作实体
                entity_unload(valid_IDs);
                //重置事件信息
                config["ID_set"] = valid_IDs;
                //广播事件(包含对事件发送者的回复)
                event_broadcast(event);
            }
            //若为转移分支事件
            else if (tag == "Transfer" || tag == "Receiver")
            {

            }
            //若为请求/命令分支事件
            else if (tag == "Request" || tag == "Command")
            {
                //若目标实体ID字段字段无效
                if (!config_checker.field_check<vector<int64_t>>(config, "target_ID"))
                    return;
                //获取目标实体ID
                uint64_t target_ID = config["target_ID"].get<uint64_t>();

                //获取目标实体索引
                int64_t entity_index = entity_record_seek(target_ID);
                //若索引无效
                if (entity_index < 0)
                    return;

                //简化表示路径
                auto& target_entity = entity_set[entity_index].entity;
                //若目标实体类型错误
                if (target_entity.type_get() != target_type)
                    return;

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
                            if(minion_ID == target_ID)
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

