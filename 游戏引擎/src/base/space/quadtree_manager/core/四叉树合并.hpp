#pragma once
#include "src/base/space/quadtree_manager/函数预声明.h"

//四叉树合并_____收集候选组合
template<typename T>
void Quadtree_Manager<T>::quedtree_merge_collect(vector<vector<tree_record<T>*>>& receiver)
{
    //简化表示路径
    auto& max_tree_size = settings.max_tree_size;
    auto& min_tree_size = settings.min_tree_size;
    auto& tree_group = X_sequence;

    //四叉树扩大级数存储
    int expand_level = 0;
    //四叉树当前最大可扩大大小存储
    int max_expandable_size = largest_tree_size;
    //若当前最大四叉树大小已达上限
    if (max_expandable_size == max_tree_size)
        max_expandable_size /= 2;

    //计算最大可扩大四叉树扩大级数
    for (int temp = max_expandable_size; (temp /= 2) >= min_tree_size;)
        expand_level++;

    //不同扩大级数四叉树存储
    vector<vector<tree_record<T>*>> level_tree_group(expand_level+1);
    //四叉树大小存储
    uint64_t tree_size;

    //检测检测不同扩大级数四叉树数量
    for (int detect_time = 0; detect_time < tree_group.size(); detect_time++)
    {
        //重置四叉树扩大级数
        expand_level = 0;
        //重置四叉树大小
        tree_size = tree_group[detect_time]->size;
        //若当前四叉树超过可扩大大小则略过
        if (tree_size > max_expandable_size)
            continue;
        //计算当前四叉树大小情况
        for (; (tree_size /= 2) >= min_tree_size;)
            expand_level++;
        //将当前四叉树放入相应分组
        level_tree_group[expand_level].push_back(tree_group[detect_time]);
    }

    //数量筛选通过四叉树存储——同级四叉树
    vector<vector<tree_record<T>*>> level_qualified_tree{};

    //筛选数量符合可合并最低条件的四叉树组合
    for (int filter_index = 0; filter_index < level_tree_group.size(); filter_index++)
    {
       //若当前扩大级数四叉树数量大于等于4
       //则将其放入可合并集合
        if (level_tree_group[filter_index].size() >= 4)
            level_qualified_tree.push_back(level_tree_group[filter_index]);
    }

    //相邻四叉树存储
    vector<tree_record<T>*> next_trees{};

    //筛选相邻四叉树数量符合要求的四叉树
    for (int group_index = 0; group_index < level_qualified_tree.size(); group_index++)
    {
        //简化表示路径
        auto& candidate_trees = level_qualified_tree[group_index];

        //筛选可合并组合
        for (int filter_index = 0; filter_index < candidate_trees.size(); filter_index++)
        {
            //获取当前主体四叉树信息
            tree_record<T>* center_tree = candidate_trees[filter_index];

            //重置相邻四叉树集合
            next_trees.clear();
            //候选相邻树筛选
            next_tree_seek(next_trees, center_tree, &candidate_trees);

            //虽然中心四叉树被包含其中
            //但其在筛选过程中会被排除
            //故相邻树数量大于三即可
            if (next_trees.size() >= 3)
            {
                //添加主体四叉树
                next_trees.push_back(center_tree);
                //交换主体四叉树位置
                swap(next_trees.front(),next_trees.back());
                //记录筛选出的组合
                receiver.push_back(next_trees);
            }
        }
    }
}

//四叉树合并_____精确筛选组合
template<typename T>
void Quadtree_Manager<T>::quedtree_merge_filter(const vector<vector<tree_record<T>*>>& candidate,
    vector<vector<tree_record<T>*>>& receiver)
{
    //筛选思路：
            //穷举法----提前计算各种可能出现合并四叉树中心
            //然后不同分组各自遍历所有情况
            //首先满足条件的四叉树组合即为待合并四叉树组
    //筛选条件：
            //若等大的四棵四叉树的根节点坐标平均数
            //能够满足一种新四叉树根节点可能坐标
            //即为满足条件的四叉树组合

    //新四叉树可能根节点坐标存储
    vector<coord_double> possible_root{};
    //主体四叉树坐标存储
    coord_double center_tree_root{ 0.5, 0.5 };
    //主体四叉树大小存储
    uint64_t center_tree_size;
    //根节点坐标偏移
    uint64_t offset;

    // ===== 修改点：改为按指针去重 =====
    unordered_set<tree_record<T>*> used_trees{};

    //计算可能根节点坐标并筛选符合条件的四叉树
    for (int calcu_index = 0; calcu_index < candidate.size(); calcu_index++)
    {
        //简化表示路径
        auto& tree_group = candidate[calcu_index];

        //循环跳过变量
        bool is_loop_skip = false;
        //检测当前中心四叉树是否已被排除（按指针）
        if (used_trees.count(tree_group.front()))
            is_loop_skip = true;
        //若循环跳过变量为真则进入下一循环
        if (is_loop_skip == true)
            continue;

        //重置主体四叉树坐标
        center_tree_root = tree_group.front()->root;
        //重置主体四叉树大小
        center_tree_size = tree_group.front()->size;
        //重置根节点坐标偏移
        offset = center_tree_size / 2;

        //重置可能根节点坐标存储
        possible_root.clear();
        //计算新四叉树可能根节点坐标(共四组情况)
        possible_root.push_back({ center_tree_root.X + offset, center_tree_root.Y + offset });
        possible_root.push_back({ center_tree_root.X + offset, center_tree_root.Y - offset });
        possible_root.push_back({ center_tree_root.X - offset, center_tree_root.Y + offset });
        possible_root.push_back({ center_tree_root.X - offset, center_tree_root.Y - offset });

        //四叉树索引集合
        int indexs[3];
        //初始化索引集合
        for (int time = 0; time < 3; time++)
            indexs[time] = (time + 1);
        //四叉树集合平均根节点坐标存储
        coord_double average_root = { 0.5, 0.5 };

        //循环终止变量
        bool is_mergeable_group_found = false;

        //寻找可合并组合
        for (; indexs[0] < tree_group.size() - 2 && is_mergeable_group_found == false;)
        {
            //重置四叉树集合根节点X坐标
            average_root.X = (center_tree_root.X +
                tree_group[indexs[0]]->root.X +
                tree_group[indexs[1]]->root.X +
                tree_group[indexs[2]]->root.X) / 4.0f;
            //重置四叉树集合根节点Y坐标
            average_root.Y = (center_tree_root.Y +
                tree_group[indexs[0]]->root.Y +
                tree_group[indexs[1]]->root.Y +
                tree_group[indexs[2]]->root.Y) / 4.0f;

            //筛选可合并组合
            for (int filter_index = 0; filter_index < possible_root.size(); filter_index++)
            {
                //若平均根节点坐标与可能根坐标相符
                if (average_root.X == possible_root[filter_index].X &&
                    average_root.Y == possible_root[filter_index].Y)
                {
                    //组合记录可行性标记
                    bool is_recordable = true;
                    //检查可合并组合中是否有四叉树被排除（按指针）
                    for (int filter_time = 0; filter_time < 3; filter_time++)
                    {
                        //若检查出被排除四叉树（指针在集合中）
                        if (used_trees.count(tree_group[indexs[filter_time]]))
                        {
                            //标记组合不可记录
                            is_recordable = false;
                            break;
                        }
                    }
                    
                    //若组合可记录则继续处理
                    if (is_recordable == true)
                    {
                        //四叉树拷贝缓冲区
                        vector<tree_record<T>*> buffer;
                        //拷贝目标四叉树
                        buffer.push_back(tree_group.front());
                        buffer.push_back(tree_group[indexs[0]]);
                        buffer.push_back(tree_group[indexs[1]]);
                        buffer.push_back(tree_group[indexs[2]]);
                        //记录可合并组合
                        receiver.push_back(buffer);
                        //记录排除筛选组合（插入所有四棵树的指针）
                        for (auto* ptr : buffer)
                            used_trees.insert(ptr);
                        //结束外层循环
                        is_mergeable_group_found = true;
                        //结束内层循环
                        break;
                    }
                }
            }

            // 索引自增
            indexs[2]++;

            // 校准进位（确保索引递增且不越界）
            if (indexs[2] >= static_cast<int>(tree_group.size())) {
                ++indexs[1];
                indexs[2] = indexs[1] + 1;
                if (indexs[1] >= static_cast<int>(tree_group.size()) - 1) {
                    ++indexs[0];
                    indexs[1] = indexs[0] + 1;
                    indexs[2] = indexs[1] + 1;
                }
            }
        }
    }
}

//四叉树合并总函数
template<typename T>
void Quadtree_Manager<T>::qurdtree_merge(void)
{            
    //若外界已经设置前置需求接口
    //则开始四叉树合并
    if (copy)
    {
        //简化表示路径
        auto& is_cache_enabled = settings.is_cache_enabled;

        //高速缓存状态操作标志位
        bool is_cache_operate = false;

        //若高速缓存正在生效
        if(is_cache_enabled == true)
        {
            //设置高速缓存失效
            is_cache_enabled = false;
            //设置高速缓存操作位
            is_cache_operate = true;
        }

        //分类筛选合格四叉树
        vector<vector<tree_record<T>*>> classfied_tree{};
        //确认筛选合格四叉树
        vector<vector<tree_record<T>*>> varified_tree{};
        //待合并四叉树分类
        quedtree_merge_collect(classfied_tree);
        //待合并四叉树确认筛选
        quedtree_merge_filter(classfied_tree, varified_tree);

        //新四叉树根节点坐标存储
        coord_double new_root{ 0.5,0.5 };
        //区块数据缓冲区
        vector<tree_block_data<T>*> buffer{};
        //待合并/卸载四叉树序列索引存储
        vector<int64_t> index_set{};

        //合并四叉树
        for (int merge_time = 0; merge_time < varified_tree.size(); merge_time++)
        {
            //简化表示路径
            auto& tree_group = varified_tree[merge_time];

            //重置四叉树根节点坐标
            new_root.X = (tree_group[0]->root.X + tree_group[1]->root.X +
                tree_group[2]->root.X + tree_group[3]->root.X) / 4;
            new_root.Y = (tree_group[0]->root.Y + tree_group[1]->root.Y +
                tree_group[2]->root.Y + tree_group[3]->root.Y) / 4;
            
            //创建新四叉树
            quadtree_build(new_root, tree_group.front()->size * 2);        
            //获取新四叉树
            auto& new_tree = X_sequence[quadtree_index_seek(new_root)];

            //数据拷贝指针
            tree_block_data<T>* ptr_data = nullptr;
            //待合并四叉树范围存储
            coord_range merged_tree_range{};
            //重置待合并四叉树索引集合
            index_set.clear();
            //复制区块数据
            for (int seek_time = 0; seek_time < tree_group.size(); seek_time++)
            {
                //简化表示路径
                auto& merged_tree = tree_group[seek_time];
                //计算待合并四叉树管理范围
                merged_tree->tree->manage_range_calcu(merged_tree_range, merged_tree->root, merged_tree->size);
                //计算待合并四叉树序列索引
                index_set.push_back(quadtree_index_seek(merged_tree->root));
                //重置区块数据缓冲区
                buffer.clear();
                //范围查询该范围内所有区块信息
                //此处使用不稳定查询保证不创建新区块
                tree_group[seek_time]->tree->range_seek(buffer, merged_tree_range,false);
                //使用稳定查询
                //拷贝区块数据
                for (int copy_time = 0; copy_time < buffer.size(); copy_time++)
                {
                    //重置数据拷贝指针
                    ptr_data = nullptr;
                    //稳定查询创建新区块
                    new_tree->tree->block_seek(ptr_data, buffer[copy_time]->node,true);
                    //复制区块数据
                    copy(*ptr_data, *buffer[copy_time]);
                }
            }

            //卸载已经被合并的四叉树
            quadtree_unload(index_set);
        }

        //若高速缓存操作位为真
        if (is_cache_operate == true)
            //设置高速缓存生效
            is_cache_enabled = true;
    }
}

