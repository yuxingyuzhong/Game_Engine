#pragma once
#include "src/base/space/Quadtree_Manager/函数预声明.h"

//引擎命名空间
namespace engine
{
    //直属四叉树查找
    template<typename T>
    tree_record<T>* Quadtree_Manager<T>::quadtree_inclusion_seek(const coord2D_int& target)
    {
        //临时树边界存储
        coord2D_range tree_range{};

        //简化表示路径
        auto& is_cache_enabled = settings.is_cache_enabled;
        auto& records = tree_cache.records;
        auto& ranges = tree_cache.ranges;

        //若高速缓存启用且存在高速缓存
        if (is_cache_enabled && records.size() > 0)
        {
            for (int seek_time = 0; seek_time < records.size(); seek_time++)
            {
                //简化表示路径
                auto& now_tree = records[seek_time];
                //读取当前候选四叉树边界
                tree_range = ranges[seek_time];

                //若坐标完全位于候选四叉树管辖范围则为直属四叉树
                if (target.X >= tree_range.left && target.X <= tree_range.right &&
                    target.Y >= tree_range.down && target.Y <= tree_range.up)
                {
                    //若当前条目非第一条目
                    if (seek_time > 0)
                    {
                        //简化表示路径
                        auto& last_tree = records[seek_time - 1];
                        //将当前条目高速缓存向前进位
                        std::swap(now_tree, last_tree);
                        std::swap(ranges[seek_time], ranges[seek_time - 1]);
                        //递减索引保证返回条目正确
                        seek_time--;
                    }
                    //返回高速缓存条目
                    return records[seek_time];
                }
            }
        }

        //若不存在高速缓存
        //或高速缓存查找失败
        //则调用四叉树序列

        //简化表示路径
        auto& tree_group = X_sequence;
        auto& cache_active_threshold = settings.cache_active_threshold;
        auto& max_cache_records = settings.max_cache_records;

        //匹配直属四叉树
        for (int seek_time = 0; seek_time < tree_group.size(); seek_time++)
        {
            //简化表示路径
            auto& ptr_tree = tree_group[seek_time];
            //计算当前候选四叉树边界
            ptr_tree->tree->manage_range_calcu(tree_range, ptr_tree->root, ptr_tree->size);

            //若坐标完全位于候选四叉树管辖范围则为直属四叉树
            if (target.X >= tree_range.left && target.X <= tree_range.right &&
                target.Y >= tree_range.down && target.Y <= tree_range.up)
            {
                //若高速缓存已启用且四叉树数量到达阈值
                if (is_cache_enabled && tree_group.size() >= cache_active_threshold)
                {
                    //若缓存条目已达上限
                    if (records.size() == max_cache_records)
                    {
                        records.pop_back();
                        ranges.pop_back();
                    }
                    //记录缓存条目
                    records.push_back(tree_group[seek_time]);
                    ranges.push_back(tree_range);
                }
                //返回查找结果
                return tree_group[seek_time];
            }
        }

        //若查找失败则返回空指针
        return nullptr;

    }

    //四叉树序列索引查找
    template<typename T>
    int64_t Quadtree_Manager<T>::quadtree_index_seek(const coord2D_double& root)
    {
        //简化表示路径
        auto& tree_group = X_sequence;
        //获取根节点X轴坐标与目标坐标相同的索引范围
        std::pair<int, int> range = range_binary_search(tree_group.begin(), tree_group.end(),
            root.X, std::ranges::greater(), [](auto* p) { return p->root.X; });

        //检测是否存在符合要求的四叉树
        for (int begin = range.first, end = range.second; begin <= end; begin++)
        {
            //若根节点坐标相同则返回当前索引
            if (root == tree_group[begin]->root)
                //返回需要索引
                return begin;
        }

        //若查找失败则返回无效索引
        return -1;
    }

    //四叉树创建
    template<typename T>
    void Quadtree_Manager<T>::quadtree_build(coord2D_double root, uint64_t tree_size)
    {
        //简化表示路径
        auto& tree_group = X_sequence;

        //分配新四叉树节点内存
        tree_record<T>* new_tree = new(std::nothrow) tree_record<T>;
        //若内存分配失败则直接返回
        if (new_tree == nullptr)
        {
            delete new_tree;
            return;
        }
        //分配新四叉树内存
        new_tree->tree = new(std::nothrow) Quadtree<T>(tree_size, root);
        //若内存分配失败则直接返回
        if (new_tree->tree == nullptr)
        {
            delete new_tree->tree;
            return;
        }

        //记录新四叉树大小
        new_tree->size = tree_size;
        //记录新四叉树根节点坐标
        new_tree->root = root;
        //设置新四叉树大小上限
        new_tree->tree->set_max_size(settings.max_tree_size);
        //设置新四叉树最小区块单元大小
        new_tree->tree->set_block_size(settings.block_size);
        //设置新四叉树回调管理函数
        auto manage = [this](const coord2D_double& root, const coord2D_int& seek)->bool
            { return this->tree_expand_approve(root, seek); };
        new_tree->tree->set_callback_manage(manage);

        //将新四叉树放入四叉树序列
        tree_group.push_back(new_tree);

        //按根节点X坐标降序排序
        std::ranges::sort(tree_group.begin(), tree_group.end(),
            std::ranges::greater(), [](const tree_record<T>* node) { return node->root.X; });
    }

    //四叉树卸载
    template<typename T>
    void Quadtree_Manager<T>::quadtree_unload(std::vector<int64_t>& index_set)
    {
        //简化表示路径
        auto& tree_group = X_sequence;
        auto& records = tree_cache.records;
        auto& ranges = tree_cache.ranges;

        //降序排列防止索引失效
        std::ranges::sort(index_set.begin(), index_set.end(), std::ranges::greater());
        
        //卸载四叉树
        for (int unload_time = 0; unload_time < index_set.size(); unload_time++)
        {
            //若索引无效读取下一索引
            if (index_set[unload_time] < 0)
                continue;
            //获取四叉树记录
            auto& record = tree_group[index_set[unload_time]];

            //若存在高速缓存
            //则检查卸载对象是否位于高速缓存
            for (int seek_time = 0; seek_time < records.size(); seek_time++)
            {
                //若指针地址相同则成功匹配
                if (records[seek_time] == record)
                {
                    //卸载高速缓存
                    records.erase(records.begin() + seek_time);
                    ranges.erase(ranges.begin() + seek_time);
                    //结束循环
                    break;
                }
            }

            // ---------- 释放资源 ----------
            if (record->tree != nullptr)
            {
                delete record->tree;
                record->tree = nullptr;
            }
            //删除节点本身
            delete record;
            //从四叉树序列中移除
            tree_group.erase(tree_group.begin() + index_set[unload_time]);
        }
    }

    //卸载——根节点重载
    template<typename T>
    void Quadtree_Manager<T>::quadtree_unload(const std::vector<coord2D_double>& root_set)
    {
        //简化表示路径
        auto& tree_group = X_sequence;
        auto& records = tree_cache.records;
        auto& ranges = tree_cache.ranges;

        //卸载四叉树
        for (int unload_time = 0; unload_time < root_set.size(); unload_time++)
        {
            //获取四叉树记录读取索引
            int64_t index = quadtree_index_seek(root_set[unload_time]);
            //若索引无效则读取下一索引
            if (index < 0)
                continue;

            //获取四叉树记录
            auto& record = tree_group[index];

            //若存在高速缓存
            //则检查卸载对象是否位于高速缓存
            if (records.size() > 0)
            {
                //检查高速缓存
                for (int seek_time = 0; seek_time < records.size(); seek_time++)
                {
                    //若指针地址相同则成功匹配
                    if (records[seek_time] == record)
                    {
                        //卸载高速缓存
                        records.erase(records.begin() + seek_time);
                        ranges.erase(ranges.begin() + seek_time);
                        //结束循环
                        break;
                    }
                }
            }

            // ---------- 释放资源 ----------
            if (record->tree != nullptr)
            {
                delete record->tree;
                record->tree = nullptr;
            }
            //删除节点本身
            delete record;
            //从序列中移除记录
            tree_group.erase(tree_group.begin() + index);
        }
    }

}

