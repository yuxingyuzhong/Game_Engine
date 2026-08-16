#pragma once
//预编译头
#include "common/前置头文件包含.h"
//获取预定义事件类型
#include "common/types/事件类型.h"
//获取事件终端
#include "src/core/event/Event_Terminal/事件终端.h"
//获取动态实体
#include "src/core/entity/Dynamic_Entity/动态实体.h"
//获取预定义sol2库类型别名
#include "common/external/Sol2/sol类型别名.h"
//获取配置检查器
#include "src/tools/Non_GUI/Config_Checker/配置检查器.h"
//获取辅助算法(如binary_search)
#include "src/tools/Non_GUI/Auxi_Algorithm/算法辅助工具.h"

namespace engine
{
    //实体管理器
    class Entity_Manager
    {
    private:
        //实体记录
        struct entity_record
        {
           //实体ID记录
           int64_t ID{};
           //实体
           std::shared_ptr<Dynamic_Entity> entity{};
        };
        //从属权限
        struct ownership_acl
        {
            //权限拥有实体
            std::string master{};
            //合法从属类型
            std::vector<std::string> minion_set{};
        };
        //从属记录
        struct minion_record
        {
            //上级实体ID
            uint64_t master;
            //从属实体ID
            std::vector<uint64_t> minion_set{};
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

        //决策树加载路径集合
        std::unordered_map<std::string,std::string> decision_load_paths;

        //从属权限集合
        std::vector<ownership_acl> acl_set{};
        //从属记录集合
        std::vector<minion_record> minion_records;

        //属性槽绑定通道
        std::function<std::unordered_map<std::string, double>* (const uint64_t& ID)> bind_entry;
        //活跃实体集合
        std::vector<entity_record> entity_set{};

    public:
        //构造函数
        Entity_Manager();
        //析构函数
        ~Entity_Manager() = default;
        //注册属性槽绑定通道
        void bind_entry_register(std::function<std::unordered_map<std::string, double>*
            (const uint64_t& ID)> bind_entry);
        //事件中转站接入
        void attach(void);

        // ———— 配置相关 ————

    private:
        //配置字段检验
        bool config_field_parse(const nlohmann::json& config);
        //决策树加载路径注册
        void decision_tree_register(const std::string& entity_type,const std::string& path);
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
        void entity_unload(std::vector<uint64_t>& ID);
        //实体行动
        void entity_act(void);

        // ———— 事件相关 ————

        //外部事件处理
        void outer_event_process(std::shared_ptr<config_event> evt);
    private:
        //内部事件仲裁
        void inner_event_govern(std::shared_ptr<config_event> event);
    };

}