#include "src/base/entity/entity_manager/局部命名空间使用.h"

//事件中转接口注册
void Entity_Manager::sign_event_entry(function<void(const string& module_name,
    const vector<config_event>& needed_events,function<void(config_event* evt)> event_entry)> cb_1,
    function<void(vector<config_event*>event_set)> cb_2)
{
    //保存信息注册接口
    sign_up = cb_1;
    //构造注册参数
    auto event_entry = [this](config_event* evt)-> void
        {
            this->event_process(evt);
        };

    //订阅事件记录
    vector<config_event> needed_events{};
    //遍历哈希容器提取事件
    for (auto events:event_set)
        needed_events.push_back(events);
    //注册信息
    sign_up("Entity_Mangaer", needed_events, event_entry);
    //保存事件发送接口
    send = cb_2;
}

//初始化脚本路径注册注册
void Entity_Manager::sign_entity_script(const string& entity_type, const string& path,
    const string& script_type)
{
    //错误信息记录
    string error;
    //初始化脚本记录
    LuaScript script{};
    //若初始化脚本加载成功
    if (script.load_script(path, error))
    {
        //根据脚本类型插入对应哈希容器
        if(script_type == "initialize")
            initialize_scripts.insert({ entity_type, script });
        else if(script_type == "action")
            action_scripts.insert({ entity_type, script });
    }
    //若初始化脚本加载失败
    else
        //输出错误信息
        cout << error << endl;
}

//实体从属权限注册
void Entity_Manager::sign_owner_acl(const string& master, const vector<string>& minions)
{
    //拷贝权限数据
    acl_set.push_back({ master ,minions });
    //对新权限进行内部字典序排序
    sort(acl_set.back().minions);
    //对权限集合进行字典序排序
    sort(acl_set,less(), &ownership_acl::master);
}

//实体订阅事件注册
void Entity_Manager::sign_needed_event(const vector<config_event>& needed_events)
{
    //存储当前键值数
    int counts = event_set.size();
    for (int sign_time = 0; sign_time < needed_events.size(); sign_time++)
    {
        //简化表示路径
        auto event = needed_events[sign_time];
        //若当前事件未注册
        if (!event_set.count(event))
            event_set.insert(event);
    }
}

//实体创建接口
bool Entity_Manager::entity_build(const string& type, const int& counts)
{
    //若已存储对应类型的初始化方式
    if (initialize_scripts.count(type))
    {
        //若不存在对应分组则创建
        if (!entity_set.count(type))
            entity_set.insert({ type,{{},{}} });

        //简化表示路径
        auto& entity_group = entity_set[type];
        auto& intialize_method = initialize_scripts[type];
        auto& action_method = action_scripts[type];

        //创建对应对象
        for (int build_time = 0; build_time < counts; build_time++)
        {
            //记录新实体ID
            entity_group.ID.push_back(++entity_build_total);
            //创建新实体并初始化
            Dynamic_Entity* new_entity = new(nothrow)
                Dynamic_Entity(entity_build_total,
                    intialize_method, action_method);
            //若实体创建成功
            if (new_entity != nullptr)
            {
                //构造待注入依赖
                auto event_entry = [this](entity_event* event, vector<weak_ptr<Dynamic_Entity>>& entities)->void
                    {
                        this->entity_message(event, entities);
                    };
                //设置事件发送入口
                new_entity->sign_up_entry(event_entry);
                //构造智能指针
                entity_group.entity.push_back({});
                //接管裸指针
                entity_group.entity.back().reset(new_entity);
            }
        }

        //返回创建成功
        return true;
    }
    //若未存储对应类型的创建方式
    else
        //返回创建失败
        return false;
}

//实体卸载接口
void Entity_Manager::entity_unload(const string& type,vector<uint64_t>& ID)
{
    //若不存在对应分组则创建
    if (!entity_set.count(type))
        return;

    //简化表示路径
    auto& entity = entity_set[type].entity;
    auto& ID = entity_set[type].ID;
    //对ID进行升序排序
    sort(ID);

    for (int exam_time = 0; exam_time < ID.size(); exam_time++)
    {
        //检查是否存在对应对象
        int read_index = binary_search(ID,ID[exam_time], less());
        //若得到有效索引
        if (read_index >= 0)
        {
            for (int unload_time = exam_time;; read_index++)
            {
                //若检测出相同ID则卸载该对象
                if(ID[unload_time] == ID[read_index])
                {
                    //释放指针指针
                    entity[read_index].reset();
                    //清理元素
                    entity.erase(entity.begin() + read_index);
                    ID.erase(ID.begin() + read_index);
                    //递增删除索引
                    unload_time++;
                }
            }
        }
    }    
}

//实体通讯接口
void Entity_Manager::entity_message(entity_event* evt, vector<weak_ptr<Dynamic_Entity>>& entities)
{
    //简化表示路径
    auto& tag = evt->tag;
    auto& sender = evt->sender;
    auto& config = evt->config;

    //若为创建分支事件
    if (tag == "Build")
    {
        //获取权限集合查找索引
        int acl_index = binary_search(acl_set, sender, less(), &ownership_acl::master);
        //若读取索引有效
        if (acl_index >= 0)
        {
            //简化表示路径
            auto& acl = acl_set[acl_index];
            //检查操作是否具有合法权限
            for (int exam_time = 0; exam_time < acl.minions.size(); exam_time++)
            {
                //若检查符合权限
                if (config["target"] == acl.minions[exam_time])
                {
                    //记录实体创建数量
                    int build_counts = config["counts"];
                    //创建对应实体
                    entity_build(config["target"], build_counts);

                    //简化表示路径
                    auto& entity_group = entity_set[config["target"]].entity;
                    //向事件发送者发放指针
                    for (int fill_time = 0; fill_time < build_counts; fill_time++)
                        entities.push_back((*(entity_group.rbegin() - fill_time)));
                }
            }
        }
    }
    //若为卸载分支事件
    else if (tag == "Unload")
    {
        //获取待卸载实体ID
        vector<uint64_t> ID = config.value<vector<uint64_t>>("ID", {});
        //可卸载实体ID存储
        vector<uint64_t> actual_ID{};
        //简化表示路径
        auto& entity_group = entity_set[config["target"]].entity;
        //检查卸载操作是否合法
        for (int unload_time = 0; unload_time < entities.size(); unload_time++)
        {
            //若指针指向不同地址则排除
            if (entity_group[ID[unload_time]].get() == entities[unload_time].lock().get())
                actual_ID.push_back(ID[unload_time]); ;
            //重置智能指针
            entities[unload_time].reset();
        }
        //卸载剩余实体
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
            if (record.sender == (*evt).config["target"] &&
                record.config["target"] == (*evt).sender &&
                record.ID == (*evt).config["ID"] &&
                record.config["ID"] == (*evt).ID)
            {
                //若当前为转移事件
                //则拷贝下级实体指针至缓冲区
                if (tag == "Transfer")
                    *buffer = entities;
                //若当前为接收事件
                //则拷贝下级实体指针至内部容器
                else if (tag == "Receiver")
                {
                    //接收信息
                    entities = *buffer;
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
            transfer_set.push_back({ *evt, &entities });
    }
    //若为请求分支事件
    else if (tag == "Request")
    {
        //简化表示路径
        auto& entity_group = entity_set[config["target"]].entity;
        auto& ID = entity_set[config["target"]].ID;
        //匹配目标实体
        for (int match_time = 0; match_time < entity_group.size(); match_time++)
        {
            //若实体ID匹配成功则发送事件
            if (ID[match_time] == config["ID"])
                entity_group[match_time].get()->event_receive(evt);
        }

    }
    //若为命令分支事件
    else if (tag == "Command")
    {
        cout << "非法事件分支: Command\n请更换发送渠道\n";    
        //直接返回
        return;
    }

    //管理器内部处理完毕后发送事件
    send({ static_cast<config_event*>(evt) });
}

//实体行动接口
void Entity_Manager::entity_act(void)
{
    //遍历所有实体
    for (auto& record :entity_set)
    {
        //简化表示路径
        auto& entity_group = record.second.entity;
        //使存活实体行动
        for (int act_time = 0; act_time < entity_group.size(); act_time++)
            entity_group[act_time]->act();
    }
}

//事件处理接口
void Entity_Manager::event_process(config_event* evt)
{
    //若当前为配置事件
    if (evt->category == "Config")
    {
        //简化表示路径
        auto& config = evt->config;

        //记录目标实体类型初始化脚本
        sign_entity_script(config.value("type", "未知类型"), config.value("initialize_path","未知路径"),
            "initialize");
        //记录目标实体类型行为决策脚本
        sign_entity_script(config.value("type", "未知类型"), config.value("initialize_path", "未知路径"),
            "action");
        //记录目标实体从属权限
        sign_owner_acl(config.value("type", "未知类型"), config.value<vector<string>>("acls",{"未知权限"}));
        //若存在对应键记录目标实体订阅事件
        if(config.contains("needed_events"))
        {
            //缓冲解析结果
            vector<pair<string, string>> buffer = config.value<vector<pair<string, string>>>("needed_events", {});
            //转化为可读取事件
            vector<config_event> needed_events{};
            for (int transfer_time = 0; transfer_time < buffer.size(); transfer_time++)
                needed_events.emplace_back("", buffer[transfer_time].first,
                    buffer[transfer_time].second, json::object());
            //正式载入订阅事件记录
            sign_needed_event(needed_events);

            //构造注册参数
            auto event_entry = [this](config_event* evt)-> void
                {
                    this->event_process(evt);
                };
            //订阅事件记录
            vector<config_event> total_needed_events{};
            //遍历哈希容器提取事件
            for (auto event : event_set)
                total_needed_events.push_back(event);
            //更新注册信息
            sign_up("Entity_Mangaer", total_needed_events, event_entry);
        }
    }
    //若当前非配置事件
    else
    {
        //强制转换成需要的事件类型
        entity_event* actual_event = static_cast<entity_event*>(evt);

        //简化表示路径
        auto& tag = actual_event->tag;
        auto& sender = actual_event->sender;
        auto& ID = actual_event->ID;
        auto& config = actual_event->config;

        //若为创建分支事件
        if (tag == "Build")
            //创建对应实体
            entity_build(config["target"], config["counts"]);
        //若为其他事件
        else
        {
            //若存在目标类型实体
            if (entity_set.count(config["target"]))
            {
                //简化表示路径
                auto& entity_group = entity_set[config["target"]];
                //获取事件接收者索引
                int entity_index = binary_search(entity_group.ID, ID,
                    less());
                //若存在该接收者
                if (entity_index >= 0)
                    entity_group.entity[entity_index]->event_receive(evt);
            }
        }
    }
}