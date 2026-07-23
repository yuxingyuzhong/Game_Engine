#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "事件中转站通信结构体.h"
#include "src/base/tools/Auxi_Algorithm/算法辅助工具.h"

namespace Game_Engine
{
    //事件中转器
    class Event_Broker
    {
        //事件订阅权限结构体
        struct event_acl
        {
            //事件标签
            string tag;                
            //订阅者编号集合
            vector<uint16_t> subscriber_set{};
        };

        //事件订阅权限集合
        unordered_map<string,vector<event_acl>> acl_set{};
        //事件订阅者集合
        unordered_map<string,uint16_t> subscriber_set{};
        //事件发送入口集合
        unordered_map<uint16_t,function<void(config_event* evt)>> event_entries;

    public:
        //订阅者登记注册
        void sign_up(const string& module_name, const vector<config_event>& needed_events,
            function<void(config_event* evt)> event_entry);
        //事件接收
        void receive(vector<config_event*> event_set);
    };
}