#include "src/core/entity/entity_manager/局部命名空间使用.h"

//引擎命名空间
namespace engine
{
    //事件中转接口注册
    void Entity_Manager::register_event_interface(function<void(const std::string& module_name,
        const vector<config_event>& needed_events, function<void(shared_ptr<config_event> evt)> event_entry)> cb_1,
        function<void(vector<shared_ptr<config_event>>event_set)> cb_2)
    {
        //保存信息注册接口
        sign_up = cb_1;
        //保存事件发送接口
        event_entry = cb_2;
    }

    //信息注册
    void Entity_Manager::register_info(void)
    {
        //订阅事件集合记录
        vector<config_event> needed_events{};
        //遍历哈希容器提取事件
        for (const auto& event : event_set)
            needed_events.push_back(event);
        //构造事件发送入口
        auto event_entry = [this](shared_ptr<config_event> evt)-> void
            {
                this->event_process(evt);
            };
        //注册信息
        sign_up("Entity_Manager", needed_events, event_entry);
    }

    //初始化脚本路径注册注册
    void Entity_Manager::sign_intialize_script(const std::string& entity_type, const std::string& path)
    {
        //若当前实体类型未注册初始化脚本
        if (!initialize_scripts.count(entity_type))
        {
            //打开所有标准库
            initialize_scripts[entity_type].open_libraries();
            //加载脚本代码
            initialize_scripts[entity_type].load_file(path);
        }
    }

    //实体从属权限注册
    void Entity_Manager::sign_owner_acl(const std::string& master, const vector<std::string>& minion_set)
    {
        //拷贝权限数据
        acl_set.push_back({ master ,minion_set });
        //对新权限进行内部字典序排序
        sort(acl_set.back().minion_set, less());
        //对权限集合进行字典序排序
        sort(acl_set, less(), &ownership_acl::master);
    }

    //实体订阅事件注册
    void Entity_Manager::sign_needed_event(const vector<config_event>& needed_events)
    {
        for (int sign_time = 0; sign_time < needed_events.size(); sign_time++)
        {
            //简化表示路径
            auto event = needed_events[sign_time];
            //若当前事件未注册
            if (!event_set.count(event))
                event_set.insert(event);
        }
    }

    //实体查找
    int64_t Entity_Manager::entity_find(const int64_t& ID)
    {
        //返回查找结果
        return engine::binary_search(entity_set, ID, less(), &entity_record::ID);
    }

    //实体创建接口
    void Entity_Manager::entity_build(const std::string& type, const int& counts)
    {
        //若已存储对应类型的初始化方式
        if (initialize_scripts.count(type))
        {
            //简化表示路径
            auto& intialize_path = initialize_scripts[type];

            //创建对应对象
            for (int build_time = 0; build_time < counts; build_time++)
            {
                //创建新实体并初始化
                Dynamic_Entity* new_entity = new(nothrow)Dynamic_Entity(now_ID, intialize_path);
                //若实体创建成功
                if (new_entity != nullptr)
                {
                    //构造待注入依赖
                    auto event_entry = [this](shared_ptr<config_event> event,
                        vector<weak_ptr<Dynamic_Entity>>& entities)->void
                        {
                            this->entity_message(event, entities);
                        };
                    //设置事件发送入口
                    new_entity->event_entry_sign(event_entry);
                    //构造新实体记录
                    entity_set.push_back({ now_ID,{} });
                    //接管实体指针
                    entity_set.back().entity.reset(new_entity);
                    //更新实体ID
                    now_ID++;
                }
            }
        }
    }

    //实体创建接口
    void Entity_Manager::entity_build(const std::string& type, const vector<int64_t>& IDs)
    {
        //若已存储对应类型的初始化方式
        if (initialize_scripts.count(type))
        {
            //简化表示路径
            auto& intialize_path = initialize_scripts[type];

            //创建对应对象
            for (int build_time = 0; build_time < IDs.size(); build_time++)
            {
                //若指定ID不在自定义区间则略过
                if (IDs[build_time] >= start_ID || IDs[build_time] < 0)
                    continue;

                //创建新实体并初始化
                Dynamic_Entity* new_entity = new(nothrow)Dynamic_Entity(now_ID++, intialize_path);
                //若实体创建成功
                if (new_entity != nullptr)
                {
                    //构造待注入依赖
                    auto event_entry = [this](shared_ptr<config_event> event,
                        vector<weak_ptr<Dynamic_Entity>>& entities)->void
                        {
                            this->entity_message(event, entities);
                        };
                    //设置事件发送入口
                    new_entity->event_entry_sign(event_entry);
                    //构造新实体记录
                    entity_set.push_back({ now_ID,{} });
                    //接管实体指针
                    entity_set.back().entity.reset(new_entity);
                }
            }

            //将新建实体升序排序
            sort(entity_set, less(), &entity_record::ID);
        }
    }

    //实体卸载接口
    void Entity_Manager::entity_unload(const std::string& type, vector<int64_t>& IDs)
    {
        //对待卸载ID降序排序
        //防止迭代器失效
        sort(IDs, greater());

        //循环检查待卸载ID有效性
        for (int exam_time = 0; exam_time < IDs.size(); exam_time++)
        {
            //获取实体索引
            int64_t entity_index = entity_find(IDs[exam_time]);
            //若实体查找得到有效索引
            if (entity_find(IDs[exam_time]) >= 0)
            {
                //简化表示路径
                auto* entity = entity_set[entity_index].entity.get();
                //若实体类型匹配
                if (entity->type_get() == type)
                    entity_set.erase(entity_set.begin() + entity_index);
                //若实体类型不匹配
                else
                    continue;
            }
        }
    }

    //实体通讯接口
    void Entity_Manager::entity_message(shared_ptr<config_event> evt, vector<weak_ptr<Dynamic_Entity>>& entities)
    {
        //转换为真实事件
        shared_ptr<entity_event> actual_event = static_pointer_cast<entity_event>(evt);

        //简化表示路径
        auto& category = actual_event->category;
        auto& tag = actual_event->tag;
        auto& sender = actual_event->sender;
        auto& config = actual_event->config;

        //若事件分类为实体
        if (category == "Entity")
        {
            //若为创建分支事件
            if (tag == "Build")
            {
                //获取权限集合查找索引
                int acl_index = engine::binary_search(acl_set, sender, less(), &ownership_acl::master);
                //若查找索引有效
                if (acl_index >= 0)
                {
                    //简化表示路径
                    auto& acl = acl_set[acl_index];
                    //获取目标实体类型
                    std::string target_type = config.value<std::string>("target", {});
                    //检查操作是否具有合法权限
                    for (int match_time = 0; match_time < acl.minion_set.size(); match_time++)
                    {
                        //若成功匹配权限
                        if (target_type == acl.minion_set[match_time])
                        {
                            //记录实体创建数量
                            int build_counts = config.value<int>("counts", 0);
                            //创建对应实体
                            entity_build(target_type, build_counts);
                            //获取新建实体起始迭代器
                            auto it = entity_set.rbegin();
                            //提交新建实体指针
                            for (int submit_time = build_counts; submit_time < entity_set.size(); submit_time++)
                                entities.push_back(entity_set[submit_time].entity);
                        }
                    }
                }
            }
            //若为卸载分支事件
            else if (tag == "Unload")
            {
                //获取待卸载实体类型
                std::string type = config.value<std::string>("target", {});
                //若待卸载实体实体类型为空则返回
                if (type.empty())
                    return;

                //获取待卸载实体ID
                vector<int64_t> ID = config.value<vector<int64_t>>("ID", {});
                //若ID集合为空则返回
                if (ID.empty())
                    return;

                //若ID集合与实体指针集合大小不一致
                if (ID.size() != entities.size())
                    return;

                //可卸载实体ID存储
                vector<int64_t> actual_ID{};

                //检查卸载操作是否合法
                for (int match_time = 0; match_time < entities.size(); match_time++)
                {
                    //若指针指向相同位置则合法
                    if (entity_set[ID[match_time]].entity == entities[match_time].lock())
                        actual_ID.push_back(ID[match_time]);
                }
                //清空原容器
                entities.clear();

                //卸载合法操作实体
                entity_unload(config["target"], actual_ID);
            }
            //若为转移分支事件
            else if (tag == "Transfer" || tag == "Receiver")
            {
                //记录存在性标记位
                bool is_record_found = false;
                //检查是否已经存在转移信息记录
                for (int match_time = 0; match_time < transfer_set.size(); match_time++)
                {
                    //简化表示路径
                    auto& record = transfer_set[match_time].record;
                    auto& buffer = transfer_set[match_time].buffer;
                    auto& config = record.config;

                    //若信息记录一致则进行下一步操作
                    if (record.sender == (*actual_event).config.value<std::string>("target", {}) &&
                        record.config.value<std::string>("target", {}) == (*actual_event).sender &&
                        record.ID == (*actual_event).config.value<int64_t>("ID", -1) &&
                        record.config.value<int64_t>("ID", -1) == (*actual_event).ID)
                    {
                        //若当前为转移事件
                        //则拷贝下级实体指针至缓冲区
                        if (tag == "Transfer")
                        {
                            //缓冲实体指针
                            *buffer = entities;
                            //清空原容器
                            entities.clear();
                        }
                        //若当前为接收事件
                        //则拷贝下级实体指针至内部容器
                        else if (tag == "Receiver")
                        {
                            //接收实体指针
                            for (int receive_time = 0; receive_time < buffer->size(); receive_time++)
                                entities.push_back((*buffer)[receive_time]);
                            //清理记录
                            transfer_set.erase(transfer_set.begin() + match_time);
                        }
                        //标记记录存在
                        is_record_found = true;
                        break;
                    }
                }

                //若未查找到信息记录则创建新纪录
                if (!is_record_found)
                    transfer_set.push_back({ *actual_event, &entities });
            }
            //若为请求/命令分支事件
            else if (tag == "Request" || tag == "Command")
            {
                //获取目标实体类型
                std::string target_type = config.value<std::string>("target", {});
                //若目标实体类型未指定
                if (target_type.empty())
                    return;

                //获取目标实体ID
                uint64_t ID = config.value<int64_t>("ID", -1);
                //若目标实体ID未指定
                if (ID == -1)
                    return;

                //获取目标实体索引
                int64_t target_index = entity_find(ID);
                //若索引无效
                if (target_index < 0)
                    return;

                //简化表示路径
                auto& target_entity = entity_set[target_index].entity;
                //若目标实体类型错误
                if (target_entity.get()->type_get() != target_type)
                    return;

                //若为命令分支则进一步检测
                if (tag == "Command")
                {
                    //若未提交实体指针则直接返回
                    if (entities.empty())
                        return;
                    //匹配目标实体指针
                    for (int match_time = 0; match_time < entities.size(); match_time++)
                    {
                        //若匹配到相同地址
                        if (target_entity == entities[match_time].lock())
                            break;
                        //若实体集合内无目标实体
                        else if (match_time == entities.size() - 1)
                            //驳回本次命令
                            return;
                    }
                }

                //发送事件
                entity_set[target_index].entity.get()->event_receive(evt);
            }
        }

        //管理器内部处理完毕后发送事件
        event_entry({ evt });
    }

    //实体行动接口
    void Entity_Manager::entity_act(void)
    {
        //待卸载索引集合
        vector<int64_t> unload_index_set;
        //遍历所有实体
        for (int act_time = 0; act_time < entity_set.size(); act_time++)
        {
            //简化表示路径
            auto* entity = entity_set[act_time].entity.get();
            //若实体存活
            if (entity->is_alive())
                entity->act();
            //若实体死亡
            else
                unload_index_set.push_back(act_time);
        }

        //将待卸载实体索引降序排列
        sort(unload_index_set, greater());
        //卸载所有死亡实体
        for (int unload_time = 0; unload_time < unload_index_set.size(); unload_time++)
        {
            //简化表示路径
            auto* unload_entity = entity_set[unload_index_set[unload_time]].entity.get();
            //缓冲实体ID
            vector<int64_t> buffer = { unload_entity->ID_get() };
            //卸载目标实体
            entity_unload(unload_entity->type_get(), buffer);
        }
    }

    //事件处理接口
    void Entity_Manager::event_process(shared_ptr<config_event> evt)
    {
        //若当前为配置事件
        if (evt->category == "Config")
        {
            //简化表示路径
            auto& config = evt->config;

            //记录目标实体类型初始化脚本
            sign_intialize_script(config.value("type", "未知类型"), config.value("initialize_path", "未知路径"));
            //记录目标实体从属权限
            sign_owner_acl(config.value("type", "未知类型"), config.value<vector<std::string>>("acls", { "未知权限" }));
            //若存在对应键记录目标实体订阅事件
            if (config.contains("needed_events"))
            {
                //缓冲解析结果
                vector<pair<std::string, std::string>> buffer = config.value<vector<pair<std::string, std::string>>>("needed_events", {});
                //可读取事件记录
                vector<config_event> needed_events{};
                //转化为可读取事件
                for (int transfer_time = 0; transfer_time < buffer.size(); transfer_time++)
                    needed_events.emplace_back("", buffer[transfer_time].first,
                        buffer[transfer_time].second, json::object());
                //正式载入订阅事件记录
                sign_needed_event(needed_events);
                //注册信息
                register_info();
            }
        }
        //若当前非配置事件
        else
        {
            //转换为真实事件类型
            shared_ptr<entity_event> actual_event = dynamic_pointer_cast<entity_event>(evt);

            //简化表示路径
            auto& tag = actual_event->tag;
            auto& sender = actual_event->sender;
            auto& ID = actual_event->ID;
            auto& config = actual_event->config;

            //若为创建分支事件
            if (tag == "Build")
            {
                //若未指定实体类型
                if (config.value<std::string>("target", {}).empty())
                    return;

                //获取ID编号集合
                vector<int64_t> buffer = config.value<vector<int64_t>>("IDs", {});
                //若ID编号集合为空则非自定义编号实体
                if (buffer.empty())
                    //创建对应实体
                    entity_build(config["target"], config.value<int>("counts", 0));
                //若ID编号集合非空则为自定义编号实体
                else
                    //创建对应实体
                    entity_build(config["target"], buffer);
            }
            //若为其他事件
            else
            {
                //获取目标实体类型
                std::string target_type = config.value<std::string>("target", {});
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
                target_entity->event_receive(evt);
            }
        }
    }
}

