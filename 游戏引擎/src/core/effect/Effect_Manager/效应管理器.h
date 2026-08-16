#pragma once
//预编译头
#include "common/前置头文件包含.h"
//获取预定义事件类型
#include "common/types/事件类型.h"
//获取事件终端
#include "src/core/event/Event_Terminal/事件终端.h"
//获取效应执行时间段
#include "src/core/effect/EffectPhase/效应执行时间段.h"
//获取效应槽
#include "src/core/effect/Effect/效应.h"
//获取预定义sol2库类型别名
#include "common/external/Sol2/sol类型别名.h"
//获取配置检查器
#include "src/tools/Non_GUI/Config_Checker/配置检查器.h"
//获取辅助算法
#include "src/tools/Non_GUI/Auxi_Algorithm/算法辅助工具.h"

//脚本系统模块
namespace engine
{
    //效应管理器
    class Effect_Manager 
    {
    private:
        //效应记录
        struct effect_record
        {
            //记录合法标记
            bool is_vaild = false;
            //效应归属
            uint64_t inclusion;
            //效应编号
            uint64_t ID;
            //效应
            Prop_Effect pro_effect;
        };
        //效应组
        struct effect_group
        {
            //效应归属
            uint64_t inclusion;
            //效应组
            std::vector<effect_record*> effects{};
        };
    public:
        //构造函数
        Effect_Manager();
        //析构函数
        ~Effect_Manager() = default;
        //注册属性槽绑定通道
        void bind_entry_register(std::function<std::unordered_map<std::string, double>*
            (const uint64_t& ID)> bind_entry);
        //事件中转站接入
        void attach(void);
        
        // ———— 效应管理 ————

        //效应构建
        bool effect_build(std::shared_ptr<config_event> event);
        //效应卸载
        bool effect_unload(std::shared_ptr<config_event> event);
    private:
        //效应查找
        int64_t effect_seek(const uint64_t& ID);
        //效应重排序
        void effect_reranking(const effect_record& new_record);
    public:
        //效应执行
        void run_effects(EffectPhase phase); 

    public:
        //事件终端
        Event_Terminal event_terminal;
    private:
        //事件发送密钥
        int64_t acl_key = 0;

        //属性槽绑定通道
        std::function<std::unordered_map<std::string, double>* (const uint64_t& ID)> bind_entry;

        //效应起始ID
        uint64_t start_ID = 1;
        //回收ID集合
        std::vector<uint64_t> recycle_ID_set;

        //空闲索引集合
        std::vector<uint64_t> free_index_set{};
        //效应索引映射
        std::unordered_map<uint64_t, uint64_t> effect_index_map{};

        //绝对优先执行效应边界标记
        int64_t abso_prior_index_end = -1;
        //分组效应集合
        std::vector<effect_group> group_effect_set;
        //效应总集合
        std::vector<effect_record> effect_set;

        //效应配置检查器
        Config_Checker config_checker;

        //外部事件处理
        void outer_event_process(std::shared_ptr<config_event> event);
        //内部事件仲裁
        void inner_event_govern(std::vector<std::shared_ptr<config_event>> events);
    };
}
