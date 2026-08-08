#pragma once
#include "common/前置头文件包含.h"
#include "common/types/事件类型.h"
#include "src/core/event/Event_Terminal/事件终端.h"
#include "src/tools/Config_Checker/配置检查器.h"
#include "src/core/entity/Dynamic_Entity/动态实体.h"
#include "src/tools/Auxi_Algorithm/算法辅助工具.h"
#include "common/external/Sol2/sol类型别名.h"

namespace engine
{
    //实体管理器
    class Entity_Manager
    {
    private:
        //定义从属
        using minion = std::weak_ptr<Dynamic_Entity>;
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
            //转移事件记录
            std::shared_ptr<tracked_event> event;
            //转移从属实体记录
            std::vector<minion>* minions;
        };
    private:
        //订阅事件集合
        std::unordered_set<config_event> event_map{};
    public:
        //事件终端
        Event_Terminal event_terminal;
    private:
        //权限密钥
        int64_t acl_key = 0;

        //配置检查器
        Config_Checker config_checker;

        //起始实体ID
        int64_t start_ID = 10000;
        //当前实体ID
        int64_t now_ID = 10000;

        //初始化脚本集合
        std::unordered_map<std::string,LuaState> initialize_scripts;
        //从属权限集合
        std::vector<ownership_acl> acl_set{};
        //从属转移信息集合
        std::vector<ownership_transfer> transfer_records{};
        //活跃实体集合
        std::vector<entity_record> entity_set{};

    public:
        //构造函数
        Entity_Manager();
        //析构函数
        ~Entity_Manager() = default;

        // ———— 配置相关 ————

    private:
        //配置字段检验
        bool config_field_parse(const nlohmann::json& config);
        //实体初始化脚本注册
        void intialize_script_register(const std::string& entity_type,const std::string& path);
        //实体从属权限注册
        void owner_acl_register(const std::string& master,const std::vector<std::string>& minion_set);

        // ———— 实体相关 ———— 
        
        //实体查找
        int64_t entity_find(const int64_t& ID);
    public:
        //实体创建
        void entity_build(const std::string& type, const int& counts);
        //实体创建 —— ID集合重载
        void entity_build(const std::string& type,const std::vector<int64_t>& IDs);
        //实体卸载
        void entity_unload(std::vector<int64_t>& ID);
        //实体行动
        void entity_act(void);

        // ———— 事件相关 ————
    private:
        //内部事件仲裁
        void inner_event_govern(std::shared_ptr<config_event> event, std::vector<minion>& minions);
    public:
        //外部事件处理
        void outer_event_process(std::shared_ptr<config_event> evt);
    };

}