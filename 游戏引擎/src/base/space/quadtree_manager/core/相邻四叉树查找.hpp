#pragma once
#include "src/base/space/quadtree_manager/函数预声明.h"

//相邻四叉树查找_____矩形筛选
template<typename T>
void Quadtree_Manager<T>::rectangle_filter(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
    const coord_range& range,const vector<tree_record<T>*>* tree_group)
{
    /*/
    矩形筛选逻辑：选定待查找树，记录其根节点坐标，然后以根节点坐标为原点
    待查找树大小一半加全场最大四叉树大小一半为边长进行筛选
        筛选逻辑：
        第一次，以X轴范围为准，将筛选结果放置进一个临时vector
        第二次，以Y轴范围为准，在原来筛选出的临时vector里再次筛选
                直接筛选出根节点坐标在矩形内的四叉树
    /**/

    //临时筛选结果接收
    vector<tree_record<T>*> candidate{};

    //若外界未提供四叉树集合则调用X轴四叉树序列
    if (tree_group == nullptr)
        tree_group = &X_sequence;

    //检测X轴坐标
    for (int screen_time = 0; screen_time < tree_group->size(); screen_time++)
    {
        //若候选四叉树根节点X坐标位于矩形筛选范围内
        //且与主体四叉树不为同一棵四叉树则满足条件

        //简化表示路径
        auto& now_tree = (*tree_group)[screen_time];
        if (now_tree->root.X >= range.left &&
            now_tree->root.X <= range.right &&
            now_tree->root != tree->root)
            candidate.push_back(now_tree);
    }

    //若未筛选出候选四叉树则直接返回
    if (candidate.size() == 0)
        return;

    //检测Y轴坐标
    for (int screen_time = 0; screen_time < candidate.size(); screen_time++)
    {
        //在原来筛选基础上再次筛选				
        //若候选四叉树根节点Y坐标位于矩形筛选范围内
        //则将该四叉树放置进接收vector中

        //简化表示路径
        auto& now_tree = candidate[screen_time];
        if (now_tree->root.Y >= range.down &&
            now_tree->root.Y <= range.up &&
            now_tree->root != tree->root)
            receiver.push_back(now_tree);
    }
}

//相邻四叉树查找___分类筛选
template<typename T>
vector<tree_record<T>*> Quadtree_Manager<T>::next_tree_classify(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
    const vector<tree_record<T>*>& candidate)
{
    /*/
    分类筛选逻辑：
             检查vector内每一个四叉树的根节点X/Y坐标与待查找树根节点X/Y坐标相减绝对值
             是否小于等于两棵四叉树的边长之和的一半。
             若X/Y中一个坐标满足条件，则进入确认筛选，若X/Y均满足条件则直接记录为相邻树
    /**/

    //确认筛选候选者四叉树存储
    vector<tree_record<T>*> verify_candidate{};

    //根节点坐标差值
    coord_int root_diff = { 0,0 };
    //筛选条件命中次数
    int filter_hits = 0;

    for (int filter = 0; filter < candidate.size(); filter++)
    {
        //重置筛选条件命中次数
        filter_hits = 0;
        //重新计算根节点X坐标差值
        root_diff.X = abs(tree->root.X - candidate[filter]->root.X);
        //重新计算根节点Y坐标差值
        root_diff.Y = abs(tree->root.Y - candidate[filter]->root.Y);

        //若X轴坐标差值小于等于两四叉树边长和之一半
        //则满足条件
        if (root_diff.X == (tree->size + candidate[filter]->size) / 2)
            filter_hits++;
        //若Y轴坐标差值小于等于两四叉树边长和之一半
        //则满足条件
        if (root_diff.Y == (tree->size + candidate[filter]->size) / 2)
            filter_hits++;
        //若满足两个条件则必定为对角线相邻树
        //结束其筛选
        if (filter_hits == 2)
            receiver.push_back(candidate[filter]);
        //若满足一个条件则为主体四叉树周围一排树
        //筛选进入第四级
        else if (filter_hits == 1)
            verify_candidate.push_back(candidate[filter]);
    }

    //返回确认筛选候选者
    return verify_candidate;

}

//相邻四叉树查找_____验证确认
template<typename T>
template<typename Screen>
void Quadtree_Manager<T>::next_tree_verify(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
    const vector<tree_record<T>*>& candidate, Screen way)
{
    /*/
    确认筛选逻辑：
             计算vector内的四叉树的管理范围是否与待查找树存在交集
             若有则为相邻树
    /**/

    //中心四叉树边界存储
    coord_range center_tree_range{};
    //计算中心四叉树边界
    tree->tree->manage_range_calcu(center_tree_range,tree->root,tree->size);

    //当前候选四叉树边界存储
    coord_range candidate_tree_range{};

    for (int filter_time = 0; filter_time < candidate.size(); filter_time++)
    {
        //简化表示路径
        auto& ptr_tree = candidate[filter_time];
        //重置候选相邻四叉树边界
        ptr_tree->tree->manage_range_calcu(candidate_tree_range, ptr_tree->root, ptr_tree->size);

        //重置X轴坐标交集
        float left1 = center_tree_range.left, right1 = center_tree_range.right;
        float left2 = candidate_tree_range.left, right2 = candidate_tree_range.right;
        bool is_x_next = way(right1, left2, right2, left1);

        //重置Y轴坐标交集
        float down1 = center_tree_range.down, up1 = center_tree_range.up;
        float down2 = candidate_tree_range.down, up2 = candidate_tree_range.up;
        bool is_y_next = way(up1, down2, up2, down1);

        //当一侧交集不为0即为相邻树
        if (is_x_next == true || is_y_next == true)
            receiver.push_back(candidate[filter_time]);
    }
}

//候选树筛选
template<typename T>
void Quadtree_Manager<T>::candidate_tree_filter(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
    const vector<tree_record<T>*>& candidate)
{
    //若筛选出候选四叉树
    //则进行进一步的相邻四叉树分类
    vector<tree_record<T>*> verify_candidate = next_tree_classify(receiver, tree, candidate);

    //四叉树筛选方式lambda
    auto screen_method = [](float coord_1, float coord_2, float coord_3, float coord_4) -> bool
        {
            if (coord_1 >= coord_2 - 1 && coord_3 >= coord_4 - 1)
                return true;
            else
                return false;
        };
    //PS:该lambda的筛选逻辑会筛选重叠四叉树
       //目的为四叉树扩大回调管理出进一步锁定重叠四叉树
       //故此处逻辑不需修改

    //相邻四叉树确认
    next_tree_verify(receiver,tree,verify_candidate, screen_method);
}

//相邻四叉树查找总函数
template<typename T>
void Quadtree_Manager<T>::next_tree_seek(vector<tree_record<T>*>& receiver, const tree_record<T>* tree, 
    const vector<tree_record<T>*>* tree_group)
{
    //矩形四叉树筛选结果存储
    vector<tree_record<T>*> rectan_trees{};
    //筛选以四叉树为中心的矩形范围内是否存在相邻四叉树
    coord_range tree_range{};
    //计算矩形筛选范围
    tree_range.left  = tree->root.X - (tree->size + largest_tree_size) / 2;
    tree_range.right = tree->root.X + (tree->size + largest_tree_size) / 2;
    tree_range.up    = tree->root.Y + (tree->size + largest_tree_size) / 2;
    tree_range.down  = tree->root.Y - (tree->size + largest_tree_size) / 2;
    //进行矩形筛选
    rectangle_filter(rectan_trees, tree, tree_range,tree_group);
    //若未筛选出候选四叉树或内存分配失败则直接返回
    if (rectan_trees.size() == 0)
    {
        receiver = rectan_trees;
        return;
    }

    //过滤矩形筛选结果
    candidate_tree_filter(receiver,tree,rectan_trees);
}
