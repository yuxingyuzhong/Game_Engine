#pragma once
//预编译头
#include "common/前置头文件包含.h"

//游戏引擎命名空间
namespace engine
{
    //抽象事件
    struct event
    {
        //目标接收者
        std::string target_module{};
        //事件大类
        std::string category;
        //类内标签
        std::string tag;

        //使用默认等于运算符
        bool operator==(const event&) const = default;
        //默认构造函数
        event()
        {

        }
        //含参构造函数
        event(const std::string& target_module, const std::string& category, const std::string& tag)
        {
            this->target_module = target_module;
            this->category = category;
            this->tag = tag;
        }

        //析构函数保证该结构体不可创建
        virtual ~event() = 0;

    };

    //纯虚析构函数实现
    inline event::~event() = default;

    //配置事件
    struct config_event : public event
    {
        //默认构造函数
        config_event()
        {

        }
        //含参构造函数
        config_event(const std::string& target_module, const std::string& category,
            const std::string& tag, const nlohmann::json& config)
        {
            this->target_module = target_module;
            this->category = category;
            this->tag = tag;
            this->config = config;
        }
        //默认析构函数
        ~config_event()
        {

        }
        //配置包
        nlohmann::json config;
        //使用默认等于运算符
        bool operator==(const config_event& other) const
        {
            if (this->category == other.category &&
                this->tag == other.tag &&
                this->target_module == other.target_module &&
                this->config == other.config)
                return true;
            else
                return false;
        }
    };

    //追踪事件
    struct tracked_event : public config_event
    {
        //默认构造函数
        tracked_event()
        {

        }
        //含参构造函数
        tracked_event(const std::string& target_module, const std::string& category, const std::string& tag,
            const nlohmann::json& config, const std::string& sender, const int64_t ID)
        {
            this->target_module = target_module;
            this->category = category;
            this->tag = tag;
            this->config = config;
            this->sender = sender;
            this->ID = ID;
        }
        //默认析构函数
        ~tracked_event()
        {

        }
        //发起者类型标签
        std::string sender;
        //发起者实体ID
        int64_t ID = 0;

        //使用默认等于运算符
        bool operator==(const tracked_event& other) const
        {
            if (this->category == other.category &&
                this->tag == other.tag &&
                this->target_module == other.target_module &&
                this->config == other.config &&
                this->sender == other.sender &&
                this->ID == other.ID)
                return true;
            else
                return false;
        }
    };

}

// 哈希组合工具
namespace detail 
{
    inline void hash_combine(size_t& seed, size_t val) noexcept {
        seed ^= val + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}

namespace std
{
    // 为 event 特化哈希
    template<>
    struct hash<engine::event>
    {
        size_t operator()(const engine::event& evt) const noexcept
        {
            std::hash<std::string> str_hasher;
            size_t seed = str_hasher(evt.category);
            detail::hash_combine(seed, str_hasher(evt.tag));
            detail::hash_combine(seed, str_hasher(evt.target_module));
            return seed;
        }
    };

    // ========== 修复 config_event 哈希特化 ==========
    template<>
    struct hash<engine::config_event>
    {
        size_t operator()(const engine::config_event& evt) const noexcept
        {
            std::hash<std::string> str_hasher;
            size_t seed = 0;

            // 1. 哈希基类成员（与 event 一致）
            seed = str_hasher(evt.category);
            detail::hash_combine(seed, str_hasher(evt.tag));
            detail::hash_combine(seed, str_hasher(evt.target_module));

            // 2. 哈希派生类成员 config（将 json 转为字符串再哈希）
            //    注意：dump() 可能抛出异常，但 noexcept 标记要求不抛，这里假设不会。
            //    若担心，可以捕获异常并返回一个默认值（但会破坏一致性）。
            std::string config_str = evt.config.dump();
            detail::hash_combine(seed, str_hasher(config_str));

            return seed;
        }
    };

    template<>
    struct hash<engine::tracked_event> {
        size_t operator()(const engine::tracked_event& e) const noexcept {
            std::hash<std::string> hasher;
            size_t seed = hasher(e.category);
            detail::hash_combine(seed, hasher(e.tag));
            detail::hash_combine(seed, hasher(e.target_module));
            detail::hash_combine(seed, hasher(e.config.dump()));
            detail::hash_combine(seed, hasher(e.sender));
            detail::hash_combine(seed, std::hash<uint64_t>()(e.ID));
            return seed;
        }
    };

}
