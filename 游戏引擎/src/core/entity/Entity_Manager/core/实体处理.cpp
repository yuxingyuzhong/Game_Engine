#include "../局部命名空间使用.h"
#include "src/tools/Non_GUI/Logging/日志系统.h"

//引擎命名空间
namespace engine
{
    //权限记录查找
    int64_t Entity_Manager::acl_record_seek(const std::string& master)
    {
        return binary_search(acl_set, master, less(), &minion_acl::master);
    }

    //从属记录查找
    int64_t Entity_Manager::minion_record_seek(const uint64_t& ID)
    {
        return binary_search(minion_records, ID, less(), &minion_record::master_ID);
    }

    //实体记录查找
    int64_t Entity_Manager::entity_record_seek(const uint64_t& ID)
    {
        return binary_search(entity_set, ID, less(), &entity_record::ID);
    }

    //从属记录添加
    void Entity_Manager::minion_record_add(const uint64_t& master_ID, const std::vector<uint64_t>& IDs)
    {
        //获取从属记录索引
        int64_t minion_index = minion_record_seek(master_ID);
        //若索引无效
        if (minion_index < 0)
        {
            //创建从属记录
            minion_records.push_back({ master_ID,{} });
            //获取记录索引
            minion_index = minion_records.size() - 1;
        }
        //记录从属实体信息
        for (const auto& ID : IDs)
            minion_records[minion_index].minion_IDs.push_back(ID);

        //从属标签设置
        for (auto& ID : IDs)
        {
            //获取从属实体记录索引
            int64_t entity_index = entity_record_seek(ID);
            //若返回索引无效
            if (entity_index < 0)
                continue;

            //设置上级实体标签
            entity_set[entity_index].master_ID = master_ID;
        }
    }

    //从属记录删除
    vector<uint64_t> Entity_Manager::minion_record_erase(const uint64_t& master_ID,
        const vector<uint64_t>& IDs)
    {
        //获取从属记录索引
        int64_t minion_index = minion_record_seek(master_ID);
        //若返回索引无效
        if (minion_index < 0)
            return {};

        //从属记录卸载索引集合
        vector<uint64_t> unload_minion_index{};
        //可卸载从属ID集合
        vector<uint64_t> valid_IDs{};
        //获取从属ID集合
        auto& minion_IDs = minion_records[minion_index].minion_IDs;
        for (auto& ID : IDs)
        {
            //匹配当前实体ID
            for (int match_index = 0; match_index < minion_IDs.size(); match_index++)
            {
                //若ID匹配成功
                if (minion_IDs[match_index] == ID)
                {
                    //记录合法ID索引
                    unload_minion_index.push_back(match_index);
                    //记录合法ID
                    valid_IDs.push_back(ID);
                    //结束匹配
                    break;
                }
            }
        }

        //卸载合法从属实体标签
        for (auto& ID : valid_IDs)
        {
            //获取从属实体记录索引
            int64_t entity_index = entity_record_seek(ID);
            //若返回索引无效
            if (entity_index < 0)
                continue;

            //重置上级实体标签
            entity_set[entity_index].master_ID = nullopt;
        }

        //降序排序防止迭代器失效
        sort(unload_minion_index, greater());
        //卸载实体记录
        for (int erase_time = 0; erase_time < unload_minion_index.size(); erase_time++)
            minion_IDs.erase(minion_IDs.begin() + unload_minion_index[erase_time]);
    }

    //实体创建 —— 创建数目重载
    vector<uint64_t> Entity_Manager::entity_build(const string& type, const int& counts,
        optional<uint64_t> master_ID)
    {
        //实体ID记录
        vector<uint64_t> IDs{};
        //记录实体ID
        for (int record_times = 0; record_times < counts; record_times++)
            IDs.push_back(now_entity_ID++);
        //调用ID集合重载
        entity_build(type, IDs, master_ID);
        //返回实体ID集合
        return IDs;
    }

    //实体创建 —— ID集合重载
    void Entity_Manager::entity_build(const string& type, const vector<uint64_t>& IDs,
        std::optional<uint64_t> master_ID)
    {
        //若已存储对应类型的决策树加载路径且属性槽管理器已绑定
        if (decision_load_paths.count(type) && prop_manager)
        {
            //简化表示路径
            auto& load_path = decision_load_paths[type];

            //创建对应对象
            for (int build_time = 0; build_time < IDs.size(); build_time++)
            {
                //构造新实体记录
                entity_set.push_back({ master_ID,IDs[build_time] ,{} });
                //获取新实体
                auto& new_entity = entity_set.back().entity;

                //构造待注入依赖
                auto event_entry = [this](shared_ptr<config_event> event)->void
                    {
                        //处理事件
                        this->inner_event_govern(event);
                    };
                //设置事件发送入口
                new_entity.event_terminal.send_entry_register(event_entry);
                //创建属性槽
                prop_manager->prop_slot_build(type, IDs[build_time]);
                //绑定属性槽
                new_entity.prop_slot_bind(prop_manager->prop_slot_get(IDs[build_time]));
                //检查从属权限
                for (auto& acl : acl_set)
                {
                    //若成功匹配则启用从属功能
                    if (acl.master == type)
                        new_entity.minion_function_enable();
                }
                //加载决策树
                new_entity.decision_tree_load(load_path);
            }

            //若上级实体ID不为空
            if (master_ID.has_value())
                //记录从属信息
                minion_record_add(master_ID.value(), IDs);

            //将新建实体升序排序
            sort(entity_set, less(), &entity_record::ID);
        }
    }

    //实体卸载
    void Entity_Manager::entity_unload(vector<uint64_t>& IDs)
    {
        //降序排序防止迭代器失效
        sort(IDs, greater());

        for (int unload_index = 0; unload_index < IDs.size(); unload_index++)
        {
            //获取实体索引
            int64_t entity_index = entity_record_seek(IDs[unload_index]);
            //若实体查找得到有效索引
            if (entity_index >= 0)
            {
                //简化表示路径
                auto& entity = entity_set[entity_index].entity;
                auto& master_ID = entity_set[entity_index].master_ID;
                //销毁属性槽
                prop_manager->prop_slot_build(entity.type_get(), entity.ID_get());
                //若存在上级实体
                if (master_ID.has_value())
                    //卸载上级实体从属记录
                    minion_record_erase(master_ID.value(), { IDs[unload_index] });
                //获取从属记录索引
                int64_t minion_index = minion_record_seek(IDs[unload_index]);
                //若返回索引有效
                if (minion_index >= 0)
                {
                    //获取从属记录
                    auto& minion_IDs = minion_records[minion_index].minion_IDs;
                    //卸载从属记录
                    minion_records.erase(minion_records.begin() + minion_index);
                }
                //卸载目标实体
                entity_set.erase(entity_set.begin() + entity_index);
            }
        }
    }

    //实体行动
    void Entity_Manager::entity_act(void)
    {
        //待卸载实体ID集合
        vector<uint64_t> IDs{};
        //遍历所有实体
        for (int act_index = 0; act_index < entity_set.size(); act_index++)
        {
            //简化表示路径
            auto& entity = entity_set[act_index].entity;
            //若实体存活
            if (entity.is_alive())
                entity.act();
            //若实体死亡
            else
                IDs.push_back(entity.ID_get());
        }

        //卸载目标实体
        entity_unload(IDs);
        //构造实体卸载事件
        shared_ptr<config_event> entity_unload_event(new(nothrow) config_event);
        //若内存分配失败
        if (!entity_unload_event)
        {
            Log::error("Entity_Manager::实体卸载事件构造失败\n卸载消息无法同步");
            return;
        }
        //广播事件
        event_broadcast(entity_unload_event);
    }

}
