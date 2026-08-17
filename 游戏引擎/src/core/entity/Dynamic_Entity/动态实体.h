#pragma once
//预编译头
#include "common/前置头文件包含.h"
//获取预定义事件类型
#include "common/types/事件类型.h"
//获取事件终端
#include "src/core/event/Event_Terminal/事件终端.h"
//获取实体基类
#include "src/core/entity/Entity/实体.h"
//获取C++类型Lua端注册方法
#include "common/external/Sol2/sol类型注册.h"
//获取预定义sol2库类型别名
#include "common/external/Sol2/sol类型别名.h"


//游戏引擎命名空间
namespace engine
{
	//动态实体
	class Dynamic_Entity : public Entity
	{
	private:
		//从属记录
		struct minion_record 
		{
			//从属类型
			std::string type;
			//从属ID
			uint64_t ID;

			//默认构造
			minion_record() = default;
			//含参构造
			minion_record(const std::string& type, uint64_t ID)
			{
				this->type = type;
				this->ID = ID;
			}
		};
	private:
		//通用属性槽
		std::unordered_map<std::string, double>* property_slot;
		//从属记录集合
		std::unique_ptr<std::vector<minion_record>> minion_set{};

		// ———— 事件相关 ———— 
	public:
		//事件终端
		Event_Terminal event_terminal;
	private:
		//权限密钥
		int64_t acl_key = 0;
		//决策树
		LuaState decision_tree;

	public:
		//构造函数
		Dynamic_Entity(void);
		//构造函数
		Dynamic_Entity(const int64_t& ID);
		//构造函数
		Dynamic_Entity(const int64_t& ID, const std::string& load_path);

		//禁用拷贝
		Dynamic_Entity(const Dynamic_Entity&) = delete;
		Dynamic_Entity& operator=(const Dynamic_Entity&) = delete;

		//移动构造函数
		Dynamic_Entity(Dynamic_Entity&&) = default;
		//移动赋值函数
		Dynamic_Entity& operator=(Dynamic_Entity&&) = default;

		//析构函数
		~Dynamic_Entity();

		//ID绑定
		void ID_bind(const uint64_t& ID);
		//属性槽绑定
		void prop_slot_bind(std::unordered_map<std::string, double>* ptr);
		//从属功能启用
		bool minion_function_enable(void);
		//决策树加载
		void decision_tree_load(const std::string& load_path);
		//行为决策
		virtual void act(void);

	private:
		//事件仲裁
		void event_govern(std::shared_ptr<config_event> event);
	};

}