#include "../局部命名空间使用.h"
#include "src/tools/Non_GUI/Logging/日志系统.h"

//引擎命名空间
namespace engine
{
	//构造函数
	Dynamic_Entity::Dynamic_Entity(void)
	{
		//获取权限密钥
		this->acl_key = event_terminal.acl_key_gen();
	}

	//构造函数
	Dynamic_Entity::Dynamic_Entity(const int64_t& ID) : Dynamic_Entity()
	{
		//设置实体ID编号
		this->ID = ID;
	}

	//构造函数
	Dynamic_Entity::Dynamic_Entity(const int64_t& ID, const std::string& load_path):Dynamic_Entity(ID)
	{
		//加载决策树
		decision_tree_load(load_path);
	}

	//析构函数
	Dynamic_Entity::~Dynamic_Entity()
	{

	}

	//ID绑定
	void Dynamic_Entity::ID_bind(const uint64_t& ID)
	{
		this->ID = ID;
	}

	//属性槽绑定
	void Dynamic_Entity::prop_slot_bind(unordered_map<string, double>* ptr)
	{
		//绑定属性槽
		property_slot = ptr;
	}

	//从属功能开启
	bool Dynamic_Entity::minion_function_enable(void)
	{
		//激活从属集合
		minion_set.reset(new(nothrow) vector<minion_record>());
		//若从属集合激活失败
		if (!minion_set)
		{
			Log::info("内存不足\n从属集合激活失败");
			return false;
		}
		//若正常激活则返回true
		return true;
	}

	//决策树加载
	void Dynamic_Entity::decision_tree_load(const std::string& load_path)
	{
		//为决策树打开所有标准库
		decision_tree.open_libraries();
		//注册行为决策脚本
		decision_tree.load_file(load_path);

		// ———— 成员变量注册环节 ————

		//注册通用属性槽
		decision_tree.set("pros", sol::as_table(property_slot));
		//若从属功能已启用
		if (minion_set)
		{
			//注册从属记录类型
			decision_tree.new_usertype<minion_record>(
				"minion_record",
				//构造函数
				sol::constructors<
				minion_record(),
				minion_record(const std::string&, uint64_t)
				>(),
				//成员键值对
				"type", &minion_record::type,
				"ID", &minion_record::ID
			);
			//注册从属集合
			decision_tree.set("minion_set", ref(minion_set));
		}

		//注册事件基类信息
		register_event(decision_tree);
		//注册配置事件信息
		register_config_event(decision_tree);
		//注册事件集合引用
		decision_tree.set("event_set", ref(event_terminal.event_get(acl_key)));

		//注册事件发送函数
		decision_tree.set_function("event_send", [this](shared_ptr<config_event> event)->void
			{
				this->event_govern(event);
			});
	}

	//行为决策
	void Dynamic_Entity::act(void)
	{
		//运行决策树
		decision_tree["decision"]();

		//清空事件集合
		event_terminal.clear(acl_key);
	}

	//事件仲裁
	void Dynamic_Entity::event_govern(shared_ptr<config_event> event)
	{
		//简化表示路径
		auto* evt = event.get();

		//若为从属级实体构建事件
		if (event->tag == "Build")
		{
		}
		//若为从属实体卸载事件
		else if (event->tag == "Unload")
		{
		}
		//若为实体转移事件事件
		else if (event->tag == "Transfer")
		{

		}
		//若为从属实体接收事件
		else if (event->tag == "Receive")
		{

		}

		//发送事件
		event_terminal.event_send({ event }, acl_key);
	}
}
