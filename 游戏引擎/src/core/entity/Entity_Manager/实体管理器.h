#pragma once
#include "common/前置头文件包含.h"
#include "实体管理器通信结构体.h"
#include "src/core/entity/Dynamic_Entity/动态实体.h"
#include "src/base/tools/Auxi_Algorithm/算法辅助工具.h"

namespace engine
{
    //实体管理器
    class Entity_Manager
    {
    private:
        //定义从属
        using minion = std::weak_ptr<Dynamic_Entity>;
        //定义lua状态机
        using LuaState = sol::state;

        //实体记录结构体
        struct entity_record
        {
           //实体ID记录
           int64_t ID{};
           //实体
           std::shared_ptr<Dynamic_Entity> entity{};
        };
        //从属权限结构体
        struct ownership_acl
        {
            //权限拥有实体
            std::string master{};
            //合法从属权限实体类型标签
            std::vector<std::string> minion_set{};
        };
        //从属转移信息结构体
        struct ownership_transfer
        {
            //转移信息记录
            entity_event record;
            //转移指针记录
            std::vector<minion>* buffer;
        };

        //订阅事件集合
        std::unordered_set<config_event> event_set{};
        //信息注册接口
        std::function<void(const std::string& module_name, const std::vector<config_event>& needed_events,
            std::function<void(std::shared_ptr<config_event> evt)> event_entry)> sign_up;
        //事件发送接口
        std::function<void(std::vector<std::shared_ptr<config_event>>event_set)> event_entry;

        //起始实体ID
        int64_t start_ID = 10000;
        //当前实体ID
        int64_t now_ID = 10000;

        //初始化脚本集合
        std::unordered_map<std::string,LuaState> initialize_scripts;
        //从属权限集合
        std::vector<ownership_acl> acl_set{};
        //从属转移信息集合
        std::vector<ownership_transfer> transfer_set{};
        //活跃实体集合
        std::vector<entity_record> entity_set{};

    public:
        //事件中转站注册接口
        void register_event_interface(std::function<void(const std::string& module_name,
            const std::vector<config_event>& needed_events,
            std::function<void(std::shared_ptr<config_event> evt)> events_entry)> cb_1,
            std::function<void(std::vector<std::shared_ptr<config_event>>event_set)> cb_2);
    private:
        //信息注册
        void register_info(void);
        //实体初始化脚本注册
        void sign_intialize_script(const std::string& entity_type,const std::string& path);
        //实体从属权限注册
        void sign_owner_acl(const std::string& master,const std::vector<std::string>& minion_set);
        //实体订阅事件注册
        void sign_needed_event(const std::vector<config_event>& needed_events);
        //实体查找
        int64_t entity_find(const int64_t& ID);
    public:
        //实体创建接口
        void entity_build(const std::string& type, const int& counts);
        //实体创建接口
        void entity_build(const std::string& type,const std::vector<int64_t>& IDs);
        //实体卸载接口
        void entity_unload(const std::string& type, std::vector<int64_t>& ID);
        //实体通讯接口
        void entity_message(std::shared_ptr<config_event> event, std::vector<minion>& entities);
        //实体行动接口
        void entity_act(void);
        //事件接收接口
        void event_process(std::shared_ptr<config_event> evt);
    };

}