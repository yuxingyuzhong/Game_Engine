#pragma once
#include "src/base/space/quadtree_manager/函数预声明.h"

//直属四叉树查找
template<typename T>
tree_record<T>* Quadtree_Manager<T>::direct_qurdtree_seek(const coord_int& target)
{
    //临时树边界存储
    coord_range tree_range{};
    //默认调用四叉树序列
    auto& tree_group = tree_info.ID_sequence;

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
            return tree_group[seek_time];
    }

    //若筛选失败则返回空指针
    return nullptr;

}

//四叉树创建
template<typename T>
void Quadtree_Manager<T>::qurdtree_build(coord_double root, uint64_t tree_size)
{
    //分配新四叉树节点内存
    tree_record<T>* new_tree = new(nothrow) tree_record<T>;
    //若内存分配失败则直接返回
    if (new_tree == nullptr)
    {
        delete new_tree;
        return;
    }
    //分配新四叉树内存
    new_tree->tree = new(nothrow) Quadtree<T>(tree_size, root);
    //若内存分配失败则直接返回
    if (new_tree->tree == nullptr)
    {
        delete new_tree->tree;
        return;
    }
    //记录新四叉树编号
    new_tree->quadtree_ID = tree_build_total++;
    //记录新四叉树大小
    new_tree->size = tree_size;
    //记录新四叉树根节点坐标
    new_tree->root = root;
    //设置新四叉树大小上限
    new_tree->tree->set_max_size(tree_info.max_tree_size);
    //设置新四叉树最小区块单元大小
    new_tree->tree->set_block_size(tree_info.block_size);
    //设置新四叉树回调管理函数
    auto manage = [this](const coord_double& root, const coord_int& seek)->bool
        { return this->tree_expand_approve(root, seek); };
    new_tree->tree->set_callback_manage(manage);
    //将新四叉树放入四叉树序列
    tree_info.ID_sequence.push_back(new_tree);
    tree_info.X_sequence.push_back(new_tree);

    //对ID_sequence按编号降序排序
    sort(tree_info.ID_sequence.begin(), tree_info.ID_sequence.end(),
        greater(), &tree_record<T>::quadtree_ID);
    //对X_sequence按根节点X坐标降序排序
    sort(tree_info.X_sequence.begin(), tree_info.X_sequence.end(),
        greater(), [](const tree_record<T>* node) { return node->root.X; });
}

//四叉树卸载
template<typename T>
void Quadtree_Manager<T>::qurdtree_unload(merge_feedback<T>& tree_array)
{
    //临时指针指向待卸载四叉树
    tree_record<T>* record = nullptr;

    for (int unload_time = 0; unload_time < tree_array.ptr_tree.size(); unload_time++)
    {
        //指向待卸载四叉树
        record = tree_array.ptr_tree[unload_time];

        // 从编号序列中删除
        int idx = binary_search(tree_info.ID_sequence.begin(), tree_info.ID_sequence.end(),
            record->quadtree_ID, greater(), &tree_record<T>::quadtree_ID);
        //若为有效索引
        if (idx != -1) 
            tree_info.ID_sequence.erase(tree_info.ID_sequence.begin() + idx);

        // 从X坐标序列中删除
        idx = binary_search(tree_info.X_sequence.begin(), tree_info.X_sequence.end(),
            record->root.X, greater(), [](const tree_record<T>* n) { return n->root.X; });
        //若为有效索引
        if (idx != -1)
            tree_info.X_sequence.erase(tree_info.X_sequence.begin() + idx);

        // ---------- 释放资源 ----------
        if (record->tree != nullptr)
        {
            delete record->tree;
            record->tree = nullptr;
        }
        //删除节点本身
        delete record;
        record = nullptr;
    }
}
