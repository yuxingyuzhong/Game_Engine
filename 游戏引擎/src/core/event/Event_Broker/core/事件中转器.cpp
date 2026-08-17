#include "../局部命名空间使用.h"

//引擎命名空间
namespace engine
{
    //订阅者登记注册
    void Event_Broker::info_register(const std::string& module_name, const vector<config_event>& needed_events,
        function<void(shared_ptr<config_event> evt)> event_entry)
    {
        //订阅者ID记录
        int32_t subscriber_ID = -1;

        //若本次为二次注册
        if (mapping_set.count(module_name))
        {
            //获取订阅者ID
            subscriber_ID = mapping_set[module_name];
            //重新注册事件入口
            event_entries[subscriber_ID] = event_entry;
        }
        //若本次为首次注册
        else
        {
            //获取当前映射数目作为新订阅者编号
            subscriber_ID = mapping_set.size();
            //注册订阅者内部ID
            mapping_set.insert({ module_name, subscriber_ID });
            //注册事件入口
            event_entries.insert({ subscriber_ID ,event_entry });
        }

        //注册需要事件
        for (int register_time = 0; register_time < needed_events.size(); register_time++)
        {
            //简化表示路径
            auto& event = needed_events[register_time];

            //若事件所属分类未指定则略过
            if (event.category.empty())
                continue;

            //若事件所属分类未注册则略过
            if (!acl_set.count(event.category))
            {
                //创建该事件分类
                acl_set.insert({ event.category,{} });
                //全订阅标记初始化
                acl_set[event.category].push_back({});
            }

            //获取事件分类内部信息
            auto& acl_row = acl_set[event.category];

            //匹配事件标签
            for (int match_time = 0; match_time < acl_row.size(); match_time++)
            {
                //若事件标签匹配则订阅该事件
                if (event.tag == acl_row[match_time].tag)
                {
                    //简化表示路径
                    auto& ID_set = acl_row[match_time].ID_set;
                    //若不存在任何订阅者
                    if (ID_set.empty())
                        ID_set.push_back(subscriber_ID);
                    //若已经存在订阅者
                    else
                    {
                        //检查是否已经注册
                        for (int exam_time = 0; exam_time < ID_set.size(); exam_time++)
                        {
                            //若已经注册则处理下一事件
                            if (ID_set[exam_time] == subscriber_ID)
                                break;
                            //若未注册则注册
                            else if (exam_time == ID_set.size() - 1)
                                ID_set.push_back(subscriber_ID);
                        }
                    }

                    break;
                }
                //若匹配失败则创建该事件标签
                else if (match_time == acl_row.size() - 1)
                    acl_row.push_back({ event.tag, { subscriber_ID } });
            }
        }
    }

    //事件接收 —— 单事件重载
    void Event_Broker::receive(std::shared_ptr<config_event> event)
    {
        //简化表示路径
        auto& target_module = event->target_module;

        //若事件所属分类不存在或未注册
        if (event->category.empty() || !acl_set.count(event->category))
            return;

        //若事件标签不存在
        if (event->tag.empty())
            return;

        //若为定向发送且目标存在
        if (!target_module.empty() && mapping_set.count(target_module))
        {
            //获取目标ID
            int target_ID = mapping_set[target_module];
            //定向发送事件
            event_entries[target_ID](event);
            //处理下一事件
            return;
        }

        //简化表示路径
        auto& acl_row = acl_set[event->category];

        //授权订阅者ID记录
        vector<uint16_t> acled_IDs;

        //插入订阅事件分类内全部事件标签订阅者集合
        acled_IDs.insert(acled_IDs.end(),
            acl_row.front().ID_set.begin(), acl_row.front().ID_set.end());

        //在部分订阅集合内匹配授权订阅者
        for (int match_time = 1; match_time < acl_row.size(); match_time++)
        {
            //简化表示路径
            auto& acl = acl_row[match_time];
            //若事件标签匹配
            if (event->tag == acl.tag)
                acled_IDs.insert(acled_IDs.end(), acl.ID_set.begin(),
                    acl.ID_set.end());
        }

        //发送事件
        for (int send_time = 0; send_time < acled_IDs.size(); send_time++)
            event_entries[acled_IDs[send_time]](event);
    }

    //事件接收 —— 多事件重载
    void Event_Broker::receive(vector<shared_ptr<config_event>> event_set)
    {
        //批处理事件
        for (int process_time = 0; process_time < event_set.size(); process_time++)
            receive(event_set[process_time]);
    }
}

