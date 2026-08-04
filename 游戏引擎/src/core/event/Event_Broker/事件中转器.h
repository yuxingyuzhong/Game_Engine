#pragma once
#include "common/前置头文件包含.h"
#include "事件中转站通信结构体.h"
#include "src/base/tools/Auxi_Algorithm/算法辅助工具.h"

namespace engine
{
    //事件中转器
    class Event_Broker
    {
        //事件订阅权限结构体
        struct event_acl
        {
            //事件标签
            std::string tag;                
            //订阅者编号集合
            std::vector<int32_t> ID_set{};
        };

        //事件订阅权限集合
        std::unordered_map<std::string, std::vector<event_acl>> acl_set{};
        //事件订阅者集合
        std::unordered_map<std::string, int32_t> mapping_set{};
        //事件发送入口集合
        std::unordered_map<int32_t, std::function<void(std::shared_ptr<config_event> evt)>>
            event_entries;

    public:
        //订阅者登记注册
        void sign_up(const std::string& module_name, 
            const std::vector<config_event>& needed_events,
            std::function<void(std::shared_ptr<config_event>)> event_entry);
        //事件接收
        void receive(std::vector<std::shared_ptr<config_event>> event_set);
    };
}