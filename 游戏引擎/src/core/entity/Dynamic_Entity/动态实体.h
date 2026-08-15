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
		//定义从属标签
		using minion_tag = std::pair<int64_t, std::string>;
		//定义从属
		using minion = std::weak_ptr<Dynamic_Entity>;
	private:
		//通用属性槽
		std::unordered_map<std::string, double>* property_slot;

	    // ———— 从属相关 ————
		
		//从属标签槽--脚本端
		std::unique_ptr<std::vector<minion_tag>> minion_tag_slot;
		//从属集合--引擎端
		std::unique_ptr<std::vector<minion>> minion_set;

		//从属转移缓冲--脚本端
		std::unique_ptr<std::vector<uint64_t>> transfer_buffer;
		//从属转移缓冲--引擎端
		std::unique_ptr<std::vector<minion>> entity_buffer;

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
		Dynamic_Entity(const int64_t& ID);
		//构造函数
		Dynamic_Entity(const int64_t& ID, const std::string& load_path);
		//析构函数
		~Dynamic_Entity();

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