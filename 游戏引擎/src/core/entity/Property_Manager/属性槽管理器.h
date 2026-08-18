#pragma once
//预编译头
#include "common/前置头文件包含.h"
//获取预定义事件类型
#include "common/types/事件类型.h"
//获取事件终端
#include "src/core/event/Event_Terminal/事件终端.h"
//获取配置检查器
#include "src/tools/Non_GUI/Config_Checker/配置检查器.h"
//获取C++类型Lua端注册方法
#include "common/external/Sol2/sol类型注册.h"
//获取预定义sol2库类型别名
#include "common/external/Sol2/sol类型别名.h"
//获取引擎环境
#include "src/tools/Non_GUI/Engine_Env/引擎环境.h"
//获取路径字符串转化方法
#include "src/tools/Non_Gui/Auxi_Algorithm/路径字符串转换.h"
//获取二分查找算法
#include "src/tools/Non_GUI/Auxi_Algorithm/二分查找.h"

namespace engine
{
	//属性槽管理器
	class Property_Manager
	{
	private:
		//属性槽记录
		struct prop_record
		{
			//属性槽归属实体类型
			std::string type;
			//属性槽归属实体ID
			uint64_t ID;
			//通用属性槽
			std::unordered_map<std::string, double> property_slot;
		};
	private:
		//初始化脚本集合
		std::unordered_map<std::string, LuaState> initialize_scripts;
		//属性槽记录集合
		std::vector<prop_record> record_set;
	public:
		//事件终端
		Event_Terminal event_terminal;
	private:
		//权限密钥
		int64_t acl_key = 0;

		//配置检查器
		Config_Checker config_checker;
	public:
		//构造函数
		Property_Manager();
		//析构函数
		~Property_Manager();
		//事件中转站接入
		void attach(void);

		//读写指针获取
		Property_Manager* ptr(void);
		//只读指针获取 
		const Property_Manager* const_ptr(void) const;

		//属性槽获取
		std::unordered_map<std::string, double>* prop_slot_get(const uint64_t& ID);
		//只读属性槽获取
		const std::unordered_map<std::string, double>* const_prop_slot_get(const uint64_t& ID) const;

		//属性槽构建
		bool prop_slot_build(const std::string& type, const uint64_t& ID);
	private:
		//属性槽查找
		uint64_t prop_slot_seek(const uint64_t& ID) const;
	public:
		//属性槽卸载
		bool prop_slot_unload(const std::string& type, const uint64_t& ID);
	private:
		//事件处理
		void event_process(std::shared_ptr<config_event> event);
	};
}
