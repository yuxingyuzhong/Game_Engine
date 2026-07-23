#include "src/base/event/Event_Broker/局部命名空间使用.h"

//接收者登记注册
void Event_Broker::sign_up(const string& module_name, const vector<config_event>& needed_events,
    function<void(config_event* evt)> event_entry)
{
    //订阅者ID记录
    uint16_t subscriber_ID = -1;

    //若本次为二次注册
    if(subscriber_set.count(module_name))
    { 
        //获取订阅者ID
        uint16_t subscriber_ID = subscriber_set[module_name];
        //重新注册事件入口
        event_entries[subscriber_ID] = event_entry;
    }
    //若本次为首次注册
    else
    {
        //获取当前已注册订阅者数目
        //用作新订阅者编号
        uint16_t subscriber_ID = subscriber_set.size();
        //注册订阅者内部ID
        subscriber_set.insert({ module_name, subscriber_ID });
        //注册事件入口
        event_entries.insert({ subscriber_ID ,event_entry });
    }

    //注册需要事件
    for (int sign_time = 0; sign_time < needed_events.size(); sign_time++)
    {
        //简化表示路径
        auto& event = needed_events[sign_time];

        //若不存在当前事件所属分类
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
                auto& subscriber_set = acl_row[match_time].subscriber_set;
                //注册情况标记
                bool is_sign_up = false;
                //检查是否已经注册
                for (int exam_time = 0; exam_time < subscriber_set.size(); exam_time++)
                {
                    //若已经注册则处理下一事件
                    if(subscriber_set[exam_time] == subscriber_ID)
                    {
                        is_sign_up = true;
                        break;
                    }
                }
                //若ID未注册则进行注册
                if(is_sign_up == false)
                    acl_row[match_time].subscriber_set.push_back(subscriber_ID);
                //处理下一事件
                break;
            }
        }

        //若匹配失败则创建该事件标签
        acl_row.push_back({ event.tag, { subscriber_ID } });
    }
}

//事件接收
void Event_Broker::receive(vector<config_event*> event_set)
{
    //批处理事件
    for (int process_time = 0; process_time < event_set.size(); process_time++)
    {
        //简化表示路径
        auto& event = event_set[process_time];
        auto& target_module = event->target_module;

        //若为定向发送且目标存在
        if (!target_module.empty() && subscriber_set.count(target_module))
        {
            //获取目标ID
            int target_ID = subscriber_set[target_module];
            //定向发送事件
            event_entries[target_ID](event);
            //处理下一事件
            continue;
        }

        //若未注册该事件种类
        if (acl_set.count(event->category))
            continue;

        //简化表示路径
        auto& acl_row = acl_set[event->category];
        //授权订阅者ID记录
        vector<uint16_t> acled_IDs;

        for (int match_time = 0; match_time < acl_row.size(); match_time++)
        {
            //简化表示路径
            auto& acl = acl_row[match_time];
            //若事件标签匹配
            if(event->tag == acl.tag)
                acled_IDs.insert(acled_IDs.end(),acl.subscriber_set.begin(),
                    acl.subscriber_set.end());
        }

        //插入订阅事件分类内全部事件标签订阅者集合
        acled_IDs.insert(acled_IDs.end(),
            acl_row.front().subscriber_set.begin(), acl_row.front().subscriber_set.end());
        
        //发送事件
        for (int send_time = 0; send_time < acled_IDs.size(); send_time++)
            event_entries[acled_IDs[send_time]](event);
    }
}