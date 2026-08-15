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

    //注册属性槽绑定通道
    void Entity_Manager::bind_entry_register(std::function<std::unordered_map<std::string, double>*
        (const uint64_t& ID)> bind_entry)
    {
        this->bind_entry = bind_entry;
    }

    //事件中转站接入
    void Entity_Manager::attach(void)
    {
        //订阅事件集合记录
        vector<config_event> needed_events(event_map.begin(), event_map.end());
        //构造事件接收入口
        auto receive_entry = [this](shared_ptr<config_event> evt)-> void
            {
                this->outer_event_process(evt);
            };
        //更新接入信息
        event_terminal.attach("Entity_Manager", needed_events, receive_entry, acl_key);
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
        sort(acl_set, less(), &ownership_acl::master);
    }

    // ———— 实体相关 ———— 

    //实体查找
    int64_t Entity_Manager::entity_find(const int64_t& ID)
    {
        //返回查找结果
        return engine::binary_search(entity_set, ID, less(), &entity_record::ID);
    }

    //实体创建
    void Entity_Manager::entity_build(const string& type, const int& counts)
    {
        //实体ID记录
        vector<int64_t> IDs{};
        //记录实体ID
        for (int record_times = 0; record_times < counts; record_times++)
            IDs.push_back(now_ID++);     
        //调用ID集合重载
        entity_build(type, IDs);
    }

    //实体创建 —— ID集合重载
    void Entity_Manager::entity_build(const string& type, const vector<int64_t>& IDs)
    {
        //若已存储对应类型的决策树加载路径
        if (decision_load_paths.count(type))
        {
            //简化表示路径
            auto& load_path = decision_load_paths[type];

            //创建对应对象
            for (int build_time = 0; build_time < IDs.size(); build_time++)
            {
                //创建新实体并初始化
                shared_ptr<Dynamic_Entity> new_entity
                (new(nothrow)Dynamic_Entity(IDs[build_time]));
                //若实体创建成功
                if (new_entity != nullptr)
                {
                    //构造待注入依赖
                    auto event_entry = [this](shared_ptr<config_event> event, any others)->void
                        {
                            //简化表示路径
                            using RefType = reference_wrapper<vector<minion>>;
                            auto& ref_wrapper = std::any_cast<RefType&>(others);
                            //取出从属实体容器
                            vector<minion>& entities = ref_wrapper.get();
                            //处理事件
                            this->inner_event_govern(event, entities);
                        };
                    //设置事件发送入口
                    new_entity->event_terminal.event_entry_register(event_entry);
                    //绑定属性槽
                    new_entity->prop_slot_bind(bind_entry(IDs[build_time]));
                    //检查从属权限
                    for (auto& acl:acl_set)
                    {
                        //若成功匹配则启用从属功能
                        if (acl.master == type)
                            new_entity->minion_function_enable();
                    }   
                    //加载决策树
                    new_entity->decision_tree_load(load_path);
                    //构造新实体记录
                    entity_set.push_back({ IDs[build_time],new_entity});
                }
            }

            //将新建实体升序排序
            sort(entity_set, less(), &entity_record::ID);
        }
    }

    //实体卸载
    void Entity_Manager::entity_unload(vector<int64_t>& IDs)
    {
        //降序排序防止迭代器失效
        sort(IDs, greater());

        for (int exam_index = 0; exam_index < IDs.size(); exam_index++)
        {
            //获取实体索引
            int64_t entity_index = entity_find(IDs[exam_index]);
            //若实体查找得到有效索引
            if (entity_index >= 0)
                entity_set.erase(entity_set.begin() + entity_index);
        }
    }

    //实体行动
    void Entity_Manager::entity_act(void)
    {
        //待卸载实体ID集合
        vector<int64_t> IDs{};
        //遍历所有实体
        for (int act_index = 0; act_index < entity_set.size(); act_index++)
        {
            //简化表示路径
            auto& entity = entity_set[act_index].entity;
            //若实体存活
            if (entity->is_alive())
                entity->act();
            //若实体死亡
            else
                IDs.push_back(entity->ID_get());
        }

        //卸载目标实体
        entity_unload(IDs);
    }

    // ———— 事件相关 ————

    //外部事件处理
    void Entity_Manager::outer_event_process(shared_ptr<config_event> evt)
    {
        //若当前为配置事件
        if (evt->category == "Config")
        {
            //简化表示路径
            auto& config = evt->config;

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
            //转换为真实事件类型
            shared_ptr<tracked_event> actual_event = dynamic_pointer_cast<tracked_event>(evt);

            //简化表示路径
            auto& tag = actual_event->tag;
            auto& sender = actual_event->sender;
            auto& ID = actual_event->ID;
            auto& config = actual_event->config;

            //若为创建分支事件
            if (tag == "Build")
            {
                //若实体类型字段无效
                if (!config_checker.field_check<string>(config, "type"))
                    return;

                //获取实体创建数目
                int build_counts = config.value<int>("counts", 0);
                //若创建数目已指定
                if (build_counts != 0)
                {
                    //记录当前实体数目
                    uint64_t now_counts = entity_set.size();
                    //创建对应实体
                    entity_build(config["type"], build_counts);
                }
                else
                {
                    //获取ID编号集合
                    vector<int64_t> buffer = config.value<vector<int64_t>>("ID_set", {});
                    //若ID编号集合为空
                    if (buffer.empty())
                        return;
                    //若ID编号集合非空
                    else
                    {
                        //检查指定ID分布
                        for (auto ID : buffer)
                        {
                            //若存在指定区间外ID
                            if (ID >= start_ID || ID < 0)
                            {
                                Log::info("Entity_Manager::存在自定义区间外ID\n实体构建请求已驳回");
                                return;
                            }
                        }
                        //创建对应实体
                        entity_build(config["type"], buffer);
                    }
                }
            }
            //若为其他事件
            else
            {
                //获取目标实体类型
                string target_type = config.value<string>("type", {});
                //若未指定实体类型
                if (target_type.empty())
                    return;

                //若未指定实体ID
                if (config.value<uint64_t>("ID", 0) == 0)
                    return;

                //获取目标实体索引
                int64_t target_index = entity_find(ID);
                //若未获得有效实体索引
                if (target_index < 0)
                    return;

                //简化表示路径
                auto* target_entity = entity_set[target_index].entity.get();
                //若目标实体非指定类型
                if (target_entity->type_get() != target_type)
                    return;

                //向目标实体发送事件
                target_entity->event_terminal.event_receive(evt);
            }
        }
    }

    //内部事件仲裁
    void Entity_Manager::inner_event_govern(shared_ptr<config_event> evt,
        vector<weak_ptr<Dynamic_Entity>>& minions)
    {
        //转换为真实事件
        shared_ptr<tracked_event> actual_event = static_pointer_cast<tracked_event>(evt);

        //简化表示路径
        auto& category = actual_event->category;
        auto& tag = actual_event->tag;
        auto& sender = actual_event->sender;
        auto& config = actual_event->config;

        //若目标实体类型字段无效
        if (!config_checker.field_check<string>(config, "type"))
            return;

        //若事件分类为实体
        if (category == "Entity")
        {
            //若为创建分支事件
            if (tag == "Build")
            {
                //获取权限集合查找索引
                int acl_index = binary_search(acl_set, sender, less(), &ownership_acl::master);
                //若查找索引有效
                if (acl_index >= 0)
                {
                    //简化表示路径
                    auto& acl = acl_set[acl_index];
                    //获取目标实体类型
                    string target_type = config["type"];
                    //检查操作是否具有合法权限
                    for (int match_time = 0; match_time < acl.minion_set.size(); match_time++)
                    {
                        //若成功匹配权限
                        if (target_type == acl.minion_set[match_time])
                        {
                            //提前发布事件保证属性槽已创建
                            event_terminal.event_send({ evt }, acl_key);
                            //记录实体原始数量
                            int original_counts = entity_set.size();
                            //创建指定数量实体
                            entity_build(target_type, config.value<int>("counts", 0));
                            //记录实体当前数量
                            int now_counts = entity_set.size();
                            //发放从属实体指针
                            for (int grant_index = original_counts; grant_index < now_counts; grant_index++)
                                minions.push_back(entity_set[grant_index].entity);
                            //提前返回避免二次发布事件
                            return;
                        }
                    }
                }
            }
            //若为卸载分支事件
            else if (tag == "Unload")
            {
                //若待卸载实体ID集合字段无效
                if (!config_checker.field_check<vector<int64_t>>(config, "ID_set"))
                    return;

                //获取待卸载实体类型
                string type = config["type"];
                //获取待卸载实体ID
                vector<int64_t> IDs = config["ID_set"];

                //若ID集合与从属实体集合大小不一致
                if (IDs.size() != minions.size())
                    return;

                //可卸载实体ID存储
                vector<int64_t> actual_IDs{};
                //检查卸载操作是否合法
                for (int read_index = 0; read_index < IDs.size(); read_index++)
                {
                    //获取实体索引
                    int entity_index = entity_find(IDs[read_index]);
                    //若实体索引有效
                    if (entity_index >= 0)
                    {
                        //简化表示路径
                        auto& now_entity = entity_set[entity_index].entity;
                        //若实体类型不匹配
                        if (now_entity->type_get() != type)
                            continue;

                        for (int match_time = 0; match_time < minions.size(); match_time++)
                        {
                            //若指针指向相同位置则合法
                            if (now_entity == minions[match_time].lock())
                                actual_IDs.push_back(IDs[match_time]);
                        }
                    }
                }

                //清空原容器
                minions.clear();
                //卸载合法操作实体
                entity_unload(actual_IDs);
            }
            //若为转移分支事件
            else if (tag == "Transfer" || tag == "Receiver")
            {
                //若目标实体ID字段无效
                if (actual_event->config.value<int64_t>("ID", -1) == -1)
                    return;

                //记录创建需求标记位
                bool is_record_needed = true;
                //检查是否已经存在转移信息记录
                for (int match_time = 0; match_time < transfer_records.size(); match_time++)
                {
                    //简化表示路径
                    auto& original_event = transfer_records[match_time].event;
                    auto& minions_buffer = transfer_records[match_time].minions;

                    //若信息记录一致则进行下一步操作
                    if (original_event->sender == actual_event->config["type"].get<string>() &&
                        original_event->config["type"].get<string>() == actual_event->sender &&
                        original_event->ID == actual_event->config["ID"] &&
                        original_event->config["ID"] == actual_event->ID)
                    {
                        //标记无需创建记录
                        is_record_needed = false;

                        //若当前为转移事件
                        if (tag == "Transfer")
                        {
                            //拷贝从属实体至缓冲区
                            *minions_buffer = minions;
                            //清空原容器
                            minions.clear();
                        }
                        //若当前为接收事件
                        else if (tag == "Receiver")
                            //接收从属实体
                            minions.insert(minions.end(), minions_buffer->begin(), minions_buffer->end());

                        //清理记录
                        transfer_records.erase(transfer_records.begin() + match_time);
                        break;
                    }
                }

                //若记录创建标记为真
                if (!is_record_needed)
                    transfer_records.push_back({ actual_event, &minions });
            }
            //若为请求/命令分支事件
            else if (tag == "Request" || tag == "Command")
            {
                //获取目标实体类型
                string type = config["type"];

                //获取目标实体ID
                uint64_t ID = config.value<int64_t>("ID", -1);
                //若未定义目标ID
                if (ID == -1)
                    return;

                //获取目标实体索引
                int64_t entity_index = entity_find(ID);
                //若索引无效
                if (entity_index < 0)
                    return;

                //简化表示路径
                auto& target_entity = entity_set[entity_index].entity;
                //若目标实体类型错误
                if (target_entity->type_get() != type)
                    return;

                //若为命令分支则进一步检测
                if (tag == "Command")
                {
                    //若未提交实体指针则直接返回
                    if (minions.empty())
                        return;
                    //匹配目标实体指针
                    for (int match_time = 0; match_time < minions.size(); match_time++)
                    {
                        //若匹配到相同地址
                        if (target_entity == minions[match_time].lock())
                            break;
                        //若实体集合内无目标实体
                        else if (match_time == minions.size() - 1)
                            //驳回本次命令
                            return;
                    }
                }
                //发送事件
                target_entity->event_terminal.event_receive(evt);
            }
        }

        //管理器内部处理完毕后发送事件
        event_terminal.event_send({ evt }, acl_key);
    }

}

