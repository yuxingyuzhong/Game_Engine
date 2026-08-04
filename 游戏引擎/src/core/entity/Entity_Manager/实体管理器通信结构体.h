#pragma once
#include "common/前置头文件包含.h"
#include "src/core/event/Event_Broker/事件中转站通信结构体.h"

//游戏引擎命名空间
namespace engine
{
	//实体管理器派生事件
	struct entity_event : public config_event
	{
		//默认构造函数
		entity_event()
		{

		}
		//含参构造函数
		entity_event(const std::string& target_module, const std::string& category, const std::string& tag, 
			const nlohmann::json& config,const std::string& sender,const int64_t ID)
		{
			this->target_module = target_module;
			this->category = category;
			this->tag = tag;
			this->config = config;
			this->sender = sender;
			this->ID = ID;
		}
		//默认析构函数
		~entity_event()
		{

		}
		//发起者类型标签
		std::string sender;
		//发起者实体ID
		int64_t ID = 0;

		//使用默认等于运算符
		bool operator==(const entity_event& other) const
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

	//注册实体事件
	inline void register_entity_event(sol::state& lua)
	{
		lua.new_usertype<entity_event>("entity_event",
			sol::base_classes, sol::bases<config_event, event>(), // 多继承链
			sol::call_constructor, sol::constructors<>(), // 允许无参构造
			"sender", &entity_event::sender,
			"ID", &entity_event::ID
		);
	}

}

namespace std {
	template<>
	struct hash<engine::entity_event> {
		size_t operator()(const engine::entity_event& e) const noexcept {
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
