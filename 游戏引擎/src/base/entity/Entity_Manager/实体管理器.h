#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "实体管理器通信结构体.h"
#include "src/base/entity/Dynamic_Entity/动态实体.h"
#include "src/base/tools/Auxi_Algorithm/算法辅助工具.h"

namespace Game_Engine
{
    //实体管理器
    class Entity_Manager
    {
    private:
        //实体组结构体
        struct entity_group
        {
            //实体ID记录
            vector<uint64_t> ID{};
            //实体组
            vector<shared_ptr<Dynamic_Entity>> entity{};
        };
        //从属权限结构体
        struct ownership_acl
        {
            //权限拥有实体
            string master{};
            //合法从属权限实体类型标签
            vector<string> minions{};
        };
        //从属转移信息结构体
        struct ownership_transfer
        {
            //转移信息记录
            entity_event record;
            //转移指针记录
            vector<weak_ptr<Dynamic_Entity>>* buffer;
        };

        //订阅事件集合
        unordered_set<config_event> event_set{};
        //信息注册接口
        function<void(const string& module_name, const vector<config_event>& needed_events,
            function<void(config_event* evt)> event_entry)> sign_up;
        //事件发送接口
        function<void(vector<config_event*>event_set)> send;

        //已创建实体数量
        uint64_t entity_build_total = 0;
        //初始化脚本集合
        unordered_map<string,LuaScript> initialize_scripts;
        //行为决策脚本集合
        unordered_map<string,LuaScript> action_scripts;
        //从属权限集合
        vector<ownership_acl> acl_set{};
        //从属转移信息结构体
        vector<ownership_transfer> transfer_set{};
        //活跃实体集合
        unordered_map<string,entity_group> entity_set{};

    public:
        //事件中转站接口注册
        void sign_event_entry(function<void(const string& module_name, const vector<config_event>& needed_events,
            function<void(config_event* evt)> events_entry)> cb_1, function<void(vector<config_event*>event_set)> cb_2);
    private:
        //实体需要脚本注册
        void sign_entity_script(const string& entity_type,const string& path,
            const string& script_type);
        //实体从属权限注册
        void sign_owner_acl(const string& master,const vector<string>& minions);
        //实体订阅事件注册
        void sign_needed_event(const vector<config_event>& needed_events);
    public:
        //实体创建接口
        bool entity_build(const string& type,const int& counts);
        //实体卸载接口
        void entity_unload(const string& type,vector<uint64_t>& ID);
        //实体通讯接口
        void entity_message(entity_event* event,vector<weak_ptr<Dynamic_Entity>>& entities);
        //实体行动接口
        void entity_act(void);
        //事件接收接口
        void event_process(config_event* evt);
        
    };

}