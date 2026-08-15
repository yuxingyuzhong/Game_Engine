#include "../局部命名空间使用.h"
#include "src/tools/Non_GUI/Logging/日志系统.h"

//引擎命名空间
namespace engine
{
	//构造函数
	Dynamic_Entity::Dynamic_Entity(const int64_t& ID)
	{
		//设置实体ID编号
		this->ID = ID;
		//获取权限密钥
		this->acl_key = event_terminal.acl_key_gen();
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

	//属性槽绑定
	void Dynamic_Entity::prop_slot_bind(unordered_map<string, double>* ptr)
	{
		//绑定属性槽
		property_slot = ptr;
	}

	//从属功能开启
	bool Dynamic_Entity::minion_function_enable(void)
	{
		//激活从属标签槽
		minion_tag_slot.reset(new(nothrow) vector<minion_tag>());
		//若从属标签槽激活失败
		if (!minion_tag_slot)
		{
			Log::info("内存不足\n从属标签槽激活失败");
			return false;
		}

		//激活从属集合
		minion_set.reset(new(nothrow) vector<minion>());
		//若从属集合激活失败
		if (!minion_set)
		{
			Log::info("内存不足\n从属集合激活失败");
			return false;
		}

		//激活从属转移缓冲--脚本端
		transfer_buffer.reset(new(nothrow) vector<uint64_t>());
		//激活从属转移缓冲--引擎端
		entity_buffer.reset(new(nothrow) vector<minion>());
		//若从属转移缓冲激活失败
		if (transfer_buffer == nullptr || entity_buffer == nullptr)
		{
			Log::info("内存不足\n从属转移缓冲激活失败");
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
		if (!minion_set)
		{
			//注册从属标签槽
			decision_tree.set("minion_set", ref(*minion_tag_slot));
			//注册从属转移缓冲
			decision_tree.set("transfer_buffer", ref(*transfer_buffer));
		}

		//注册事件基类信息
		register_event(decision_tree);
		//注册配置事件信息
		register_config_event(decision_tree);
		//注册实体事件信息
		register_tracked_event(decision_tree);
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

		//若从属实体缓冲非空则获取接收从属实体
		if (!entity_buffer->empty())
			minion_set->insert(minion_set->begin(), 
				entity_buffer->begin(), entity_buffer->end());

		//若为实体转移事件事件
		if (evt->tag == "Transfer")
		{
			for (int buffer_index = 0; buffer_index < transfer_buffer->size(); buffer_index++)
			{
				//简化表示路径
				auto& target_entity_ID = (*transfer_buffer)[buffer_index];
				for (int match_time = 0; match_time < minion_set->size(); match_time++)
				{
					//简化表示路径
					auto* matched_entity = (*minion_set)[match_time].lock().get();
					//若匹配到目标对象
					if (target_entity_ID == matched_entity->ID_get())
					{
						//缓冲目标对象
						entity_buffer->push_back((*minion_set)[match_time]);
						//卸载目标对象
						minion_set->erase(minion_set->begin() + match_time);
						//校准迭代器
						match_time--;
					}
				}
			}

			//发送事件
			event_terminal.event_send({ event }, ref(*entity_buffer), acl_key);
		}
		//若为其余事件
		else
		{
			//发送事件
			event_terminal.event_send({ event }, ref(*minion_set), acl_key);

			//若为下级实体构建事件
			if (evt->tag == "Build")
			{
				//记录当前实体数量
				uint64_t counts_now = minion_set->size();

				//实体ID缓冲区
				uint64_t ID_buffer;
				//实体标签缓冲区
				string tag_buffer;
				//记录下级实体信息
				for (int record_time = 0; record_time < event->config.value<int>("counts", 0); record_time++)
				{
					//记录实体ID
					ID_buffer = (*minion_set)[counts_now + record_time].lock().get()->ID_get();
					//记录实体类型标签
					tag_buffer = (*minion_set)[counts_now + record_time].lock().get()->type_get();
					//向容器中插入实体信息
					(*minion_tag_slot).push_back({ ID_buffer, tag_buffer });
				}
			}
			//若为下级实体卸载事件
			else if (evt->tag == "Unload")
			{
				//简化表示路径
				auto& tag_slot = *minion_tag_slot;
				//获取待卸载实体ID
				vector<uint64_t> ID_set = event->config.value<vector<uint64_t>>("ID", {});
				//待卸载实体索引存储
				vector<int64_t> index_set{};
				//获取待卸载实体索引
				for (int read_time = 0; read_time < ID_set.size(); read_time++)
				{
					for (int match_time = 0; match_time < tag_slot.size(); match_time++)
					{
						//若ID成功匹配则记录索引
						if (ID_set[read_time] == tag_slot[match_time].first)
							index_set.push_back(match_time);
					}
				}
				//将索引集合降序排列
				sort(index_set, greater());
				//卸载实体信息
				for (int unload_time = 0; unload_time < index_set.size(); unload_time++)
					tag_slot.erase(tag_slot.begin() + unload_time);
			}
		}
	}
}
