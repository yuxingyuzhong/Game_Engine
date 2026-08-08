#pragma once
#include "src/core/space/Quadtree_Manager/函数预声明.h"

//引擎命名空间
namespace engine
{
    //默认构造函数
    template<typename T>
    Quadtree_Manager<T>::Quadtree_Manager()
    {

    }

    //默认析构函数
    template<typename T>
    Quadtree_Manager<T>::~Quadtree_Manager()
    {
        clear();
    }

    //数据迁移方法设置
    template<typename T>
    void Quadtree_Manager<T>::callback_register(const std::function<void(tree_chunk_data<T>& receiver,
        tree_chunk_data<T>& transmiter)>& cb_1)
    {
        //赋值函数注册
        copy = cb_1;
    }

    //四叉树最小区块单元大小设置
    template<typename T>
    void Quadtree_Manager<T>::set_block_size(const uint64_t& block_size)
    {
        //记录最小区块单元信息
        settings.block_size = block_size;
        //设置四叉树最小区块单元信息
        auto& tree_group = X_sequence;
        for (int set_time = 0; set_time < tree_group.size(); set_time++)
        {
            tree_group[set_time]->tree->set_block_size(settings.block_size);
        }
    }

    //四叉树边长上限设置
    template<typename T>
    void Quadtree_Manager<T>::set_max_size(const uint64_t& max_size)
    {
        //记录四叉树上限大小信息
        settings.max_tree_size = max_size;
        //设置四叉树上限大小信息
        auto& tree_group = X_sequence;
        for (int set_time = 0; set_time < tree_group.size(); set_time++)
            tree_group[set_time]->tree->set_max_size(settings.max_tree_size);;
    }

    //四叉树边长下限设置
    template<typename T>
    void Quadtree_Manager<T>::set_min_size(const uint64_t& min_size)
    {
        //记录四叉树下限大小信息
        settings.min_tree_size = min_size;
        //设置四叉树下限大小信息
        auto& tree_group = X_sequence;
        for (int set_time = 0; set_time < tree_group.size(); set_time++)
            tree_group[set_time]->tree->set_max_size(settings.min_tree_size);;
    }

    //高速缓存启用状态设置
    template<typename T>
    void Quadtree_Manager<T>::set_cache_state(bool enabled)
    {
        //设置高速缓存启用状态
        settings.is_cache_enabled = enabled;
    }

    //高速缓存启用阈值设置
    template<typename T>
    void Quadtree_Manager<T>::set_cache_active_threshold(const uint64_t& threshold)
    {
        //设置高速缓存启用阈值
        settings.cache_active_threshold = threshold;
    }

    //高速缓存条目上限设置
    template<typename T>
    void Quadtree_Manager<T>::set_max_cach_records(const uint64_t max_entries)
    {
        //设置高速缓存条目上限
        settings.max_cache_records = max_entries;
    }

    //四叉树管理器设置获取
    template<typename T>
    const tree_manager_settings& Quadtree_Manager<T>::settings_get(void)
    {
        //返回四叉树设置
        return settings;
    }

    //四叉树序列档案信息获取
    template<typename T>
    const std::vector<tree_record<T>*>& Quadtree_Manager<T>::records_get(void)
    {
        //返回四叉树序列档案
        return X_sequence;
    }

    //最大四叉树大小获取
    template<typename T>
    const uint64_t& Quadtree_Manager<T>::largest_size_get(void)
    {
        return largest_tree_size;
    }

    //清空高速缓存
    template<typename T>
    void Quadtree_Manager<T>::cache_clear(void)
    {
        //清空四叉树记录高速缓存
        tree_cache.records.clear();
        //清空四叉树范围高速缓存
        tree_cache.ranges.clear();
    }

    //清空所有四叉树
    template<typename T>
    void Quadtree_Manager<T>::clear(void)
    {
        //四叉树记录索引存储
        std::vector<int64_t> index_set{};
        //记录四叉树记录索引
        for (int unload_time = X_sequence.size() - 1; unload_time >= 0; unload_time--)
            index_set.push_back(unload_time);
        //卸载四叉树记录
        quadtree_unload(index_set);
    }

}


