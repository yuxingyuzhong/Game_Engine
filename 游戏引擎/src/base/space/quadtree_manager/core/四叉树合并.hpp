#pragma once
#include "src/base/space/quadtree_manager/函数预声明.h"

//四叉树合并_____收集候选组合
template<typename T>
void Quadtree_Manager<T>::quedtree_merge_collect(vector<vector<tree_record<T>*>>& receiver)
{
    //四叉树大小情况记录
    int size_case = 0;
    //四叉树当前可扩大最大大小记录
    int tree_size_may_largen = tree_size_largest_now;
    //若当前最大四叉树大小已达上限
    if (tree_size_may_largen == tree_info.max_tree_size)
        tree_size_may_largen /= 2;

    //不同大小四叉树存储
    vector<vector<tree_record<T>*>> poi_screen_tree{};
    //数量筛选通过四叉树存储——等大四叉树
    vector<vector<tree_record<T>*>> num_size_qualified_tree{};
    //相邻四叉树筛选存储
    vector<tree_record<T>*> poi_next_tree{};

    //计算四叉树大小情况
    for (; (tree_size_may_largen /= 2) >= 256;)
        size_case++;
    //初始化元素
    poi_screen_tree.resize(size_case, nullptr);

    //检测不同大小情况四叉树数量
    for (int detect_time = 0; detect_time < tree_info.num_sequence.size(); detect_time++)
    {
        //重置四叉树大小情况记录
        size_case = 0;
        //计算当前四叉树大小情况
        //数组初始索引为0故不可使用>=
        for (; (tree_info.num_sequence[detect_time] /= 2) > 256;)
            size_case++;
        //将当前四叉树放入相应分组
        poi_screen_tree[size_case].push_back(tree_info.num_sequence[detect_time]);
    }

    //筛选等大四叉树数量符合要求的四叉树大小情况
    for (int screen_time = 0; screen_time < poi_screen_tree.size(); screen_time++)
    {
        //当同种大小四叉树大于等于4才可能发生合并
        if (poi_screen_tree[screen_time].size() >= 4)
        {
            num_size_qualified_tree.push_back(poi_screen_tree[screen_time]);
        }
    }

    //筛选相邻四叉树数量符合要求的四叉树
    for (int screen_first = 0; screen_first < num_size_qualified_tree.size(); screen_first++)
    {
        //记录当前组中需要删除的节点
        unordered_set<tree_record<T>*> to_remove;
        //简化表示路径
        auto& now_tree_group = num_size_qualified_tree[screen_first];

        for (int screen_second = 0; screen_second < now_tree_group.size(); screen_second++)
        {
            //获取当前主体四叉树信息
            tree_record<T>* center_tree = now_tree_group[screen_second];
            //跳过已被标记删除的节点
            if (to_remove.count(center_tree))
                continue;

            //筛选相邻四叉树
            //PS：当前候选四叉树已经具有一定关联性
                //故不需要再调用矩形筛选

            //重置相邻四叉树集合
            poi_next_tree.clear();
            //交集筛选
            next_tree_seek_classify(poi_next_tree, center_tree, now_tree_group);

            //因为中心四叉树也被包含其中
               //所以需要筛选出3棵相邻四叉树才可能出现可合并情况(即总数量 >= 4)
            if (poi_next_tree.size() >= 4)
            {
                //寻找中心四叉树
                for (int time = 0; time < poi_next_tree.size(); time++)
                {
                    if (poi_next_tree[time]->qurdtree_ID == center_tree->qurdtree_ID)
                    {
                        //将中心四叉树放在首位
                        swap(poi_next_tree[0], poi_next_tree[time]);
                        break;
                    }
                }

                //记录筛选出的组合
                receiver.push_back(poi_next_tree);

                // 将组合中的所有节点加入待删除集合
                for (tree_record<T>* n : *poi_next_tree)
                {
                    to_remove.insert(n);
                }
            }
        }
        // 从当前组中移除所有被标记的节点
        auto& group = num_size_qualified_tree[screen_first];
        group.erase(remove_if(group.begin(), group.end(),
            [&](tree_record<T>* n) { return to_remove.count(n); }), group.end());
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
    vector<coord_double>new_tree_root{};
    //主体四叉树坐标临时存储
    coord_double center_tree_root{ 0.5,0.5 };
    //主体四叉树大小临时存储
    short size_center_tree = 256;
    //根节点坐标偏移
    short offset = size_center_tree / 2;

    //计算可能根节点坐标并筛选符合条件的四叉树
    for (int calcu_time = 0; calcu_time < candidate.size(); calcu_time++)
    {
        //重置主体四叉树坐标
        center_tree_root.X = candidate[calcu_time][0]->root.X;
        center_tree_root.Y = candidate[calcu_time][0]->root.Y;
        //重置主体四叉树大小
        size_center_tree = candidate[calcu_time][0]->size;
        //重置根节点坐标偏移
        offset = size_center_tree / 2;

        //计算新四叉树可能根节点坐标(共四组情况)
        new_tree_root.push_back({ center_tree_root.X + offset, center_tree_root.Y + offset });
        new_tree_root.push_back({ center_tree_root.X + offset, center_tree_root.Y - offset });
        new_tree_root.push_back({ center_tree_root.X - offset, center_tree_root.Y + offset });
        new_tree_root.push_back({ center_tree_root.X - offset, center_tree_root.Y - offset });

        //四叉树集合
        int tree_1 = 0;
        int tree_2 = tree_1 + 1;
        int tree_3 = tree_1 + 2;
        int tree_4 = tree_1 + 3;
        //四叉树集合根节点坐标记录
        coord_double tree_array = { 0.5,0.5 };

        //筛选符合条件的四叉树集合

        //循环条件说明：当tree_1等于poi_final_tree[calcu_time].size() - 3时
                      //由于C++索引从0开始的特性
                      //此时tree_4已经位于最后一个元素的后一位
                      //所有元素已经遍历完毕

        //循环终止变量
        bool is_while_break = false;
        for (; tree_1 < candidate[calcu_time].size() - 3 && is_while_break == false;)
        {
            auto& poi_tree = candidate[calcu_time];
            //重置四叉树集合根节点X坐标
            tree_array.X = (poi_tree[tree_1]->root.X +
                poi_tree[tree_2]->root.X +
                poi_tree[tree_3]->root.X +
                poi_tree[tree_4]->root.X) / 2;
            //重置四叉树集合根节点Y坐标
            tree_array.Y = (poi_tree[tree_1]->root.Y +
                poi_tree[tree_2]->root.Y +
                poi_tree[tree_3]->root.Y +
                poi_tree[tree_4]->root.Y) / 2;

            //筛选是否存在可合并组合
            for (int begin = 0 + calcu_time * 4, end = begin + 4; begin < end; begin++)
            {
                if (tree_array.X == new_tree_root[begin].X &&
                    tree_array.Y == new_tree_root[begin].Y)
                {
                    //临时对象拷贝
                    vector<tree_record<T>*> copy(poi_tree.begin() + begin, poi_tree.begin() + end);
                    //记录可合并组合
                    receiver.push_back(copy);
                    //结束外层循环
                    is_while_break = true;
                    //结束内层循环
                    break;
                }
            }

            //索引自增
            tree_4++;
            //索引修正
            if (tree_4 == candidate[calcu_time].size())
            {
                tree_3++;
                tree_4 = tree_3 + 1;
            }
            if (tree_3 == candidate[calcu_time].size())
            {
                tree_2++;
                tree_3 = tree_2 + 1;
            }
            if (tree_2 == candidate[calcu_time].size())
            {
                tree_1++;
                tree_2 = tree_1 + 1;
            }
        }
    }
}
       
//四叉树合并总函数
template<typename T>
void Quadtree_Manager<T>::qurdtree_merge(void)
        {            //初步筛选合格四叉树
            vector<vector<tree_record<T>*>> poi_intial_tree{};
            //最终筛选合格四叉树
            vector<vector<tree_record<T>*>> poi_final_tree{};
            //待合并四叉树初步筛选
            quedtree_merge_collect(poi_intial_tree);
            //待合并四叉树最终筛选
            quedtree_merge_filter(poi_intial_tree, poi_final_tree);
            //待合并/卸载四叉树ID存储
            merge_feedback Id_store{};
            //新四叉树根节点坐标存储
            coord_double new_tree{ 0.5,0.5 };

            //合并四叉树
            for (int merge_time = 0; merge_time < poi_final_tree.size(); merge_time++)
            {
                //简化表示路径
                auto& tree = poi_final_tree[merge_time];
                //重置四叉树根节点坐标
                new_tree.X = (tree[0]->root.X + tree[1]->root.X +
                    tree[2]->root.X + tree[3]->root.X) / 2;
                new_tree.Y = (tree[0]->root.Y + tree[1]->root.Y +
                    tree[2]->root.Y + tree[3]->root.Y) / 2;
                //创建新四叉树
                qurdtree_build(new_tree, tree[0]->size * 2);
                //记录合并/卸载四叉树编号信息
                Id_store.new_tree_ID.push_back(tree_info.num_sequence.back()->qurdtree_ID);
                Id_store.old_tree_ID.push_back(tree[0]->qurdtree_ID);
                Id_store.old_tree_ID.push_back(tree[1]->qurdtree_ID);
                Id_store.old_tree_ID.push_back(tree[2]->qurdtree_ID);
                Id_store.old_tree_ID.push_back(tree[3]->qurdtree_ID);
                //记录待卸载四叉树指针
                Id_store.ptr_tree.push_back(tree[0]);
                Id_store.ptr_tree.push_back(tree[1]);
                Id_store.ptr_tree.push_back(tree[2]);
                Id_store.ptr_tree.push_back(tree[3]);
                //临时存储范围查询所得信息
                vector<tree_block_data<T>> block_info_old_tree{};
                vector<tree_block_data<T>> block_info_new_tree{};

                //临时存储四叉树管理范围
                coord_range tree_range{};
                for (int seek_time = 0; seek_time < tree.size(); seek_time++)
                {
                    //计算当前四叉树管理范围
                    border_qurdtree_calcu(tree_range, tree[seek_time]);
                    //范围查询该范围内所有区块信息
                    block_info_old_tree = tree[seek_time]->tree->range_seek(tree_range);
                    block_info_new_tree = tree_info.num_sequence.back()->tree->range_seek(tree_range);
                    //拷贝区块信息
                    for (int copy_time = 0; copy_time < block_info_old_tree.size(); copy_time++)
                        copy(block_info_new_tree[copy_time], block_info_old_tree[copy_time]);
                }
            }

            //对Id_store按编号降序排序
            sort(Id_store.ptr_tree.begin(), Id_store.ptr_tree.end(),
                greater(), &tree_record<T>::qurdtree_ID);
            sort(Id_store.old_tree_ID.begin(), Id_store.old_tree_ID.end(),
                greater());
            sort(Id_store.new_tree_ID.begin(), Id_store.new_tree_ID.end(),
                greater());

            //回调通知上级调用者四叉树合并信息
            callback(Id_store);
            //卸载已经被合并的四叉树
            qurdtree_unload(Id_store);
        }

