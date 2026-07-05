#pragma once
#include "函数预声明.h"

//四叉树管理范围计算
template<typename T>
void Quadtree_Manager<T>::border_qurdtree_calcu(coord_range& receiver, const tree_record<T>* tree)
{
    //计算主体四叉树边界
    //四叉树中心均为浮点坐标
    //存在初始0.5偏移
    //需先行将其抵消
    receiver.left = tree->root.X - (0.5 + (tree->size / 2 - 1));
    receiver.right = tree->root.X + (0.5 + (tree->size / 2 - 1));
    receiver.down = tree->root.Y - (0.5 + (tree->size / 2 - 1));
    receiver.up = tree->root.Y + (0.5 + (tree->size / 2 - 1));
}

//直属四叉树查找
template<typename T>
tree_record<T>* Quadtree_Manager<T>::direct_qurdtree_seek(coord_int seek, vector<tree_record<T>*> seekable_set)
{
    //临时树边界存储
    coord_range range_this_tree{};
    //默认调用四叉树序列
    auto& tree_group = tree_info.num_sequence;;
    //若外界传入待查找集合则从资源中查找
    if (seekable_set.size() > 0)
        tree_group = seekable_set;

    for (int seek_time = 0; seek_time < tree_group.size(); seek_time++)
    {
        //计算当前候选四叉树边界
        border_qurdtree_calcu(range_this_tree, tree_group[seek_time]);

        //若坐标完全位于候选四叉树管辖范围则为直属四叉树
        if (seek.X >= range_this_tree.left && seek.X <= range_this_tree.right &&
            seek.Y >= range_this_tree.down && seek.Y <= range_this_tree.up)
            return tree_group[seek_time];
    }

    //若筛选失败则返回空指针
    return nullptr;

}

//四叉树创建
template<typename T>
void Quadtree_Manager<T>::qurdtree_build(coord_double root, uint16_t tree_size)
{
    //分配新四叉树节点内存
    tree_record<T>* new_tree = new(nothrow) tree_record<T>;
    //分配新四叉树内存
    new_tree->tree = new(nothrow) Quadtree<T>(tree_size, root);
    //若内存分配失败则直接返回
    if (new_tree == nullptr || new_tree->tree == nullptr)
    {
        delete new_tree->tree;
        delete new_tree;
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
        { return this->tree_largen_manage(root, seek); };
    new_tree->tree->set_callback_manage(manage);
    //将新四叉树放入四叉树序列
    tree_info.num_sequence.push_back(new_tree);
    tree_info.X_sequence.push_back(new_tree);

    //重新排序序列

    //对num_sequence按编号降序排序
    sort(tree_info.num_sequence.begin(), tree_info.num_sequence.end(),
        greater(), &tree_record<T>::qurdtree_ID);
    //对X_sequence按根节点X坐标降序排序
    sort(tree_info.X_sequence.begin(), tree_info.X_sequence.end(),
        greater(), [](const tree_record<T>* node) { return node->root.X; });
}

//四叉树卸载
template<typename T>
void Quadtree_Manager<T>::qurdtree_unload(merge_feedback<T>& tree_array)
{
    //临时指针指向待卸载四叉树
    tree_record<T>* node = nullptr;

    for (int unload_time = 0; unload_time < tree_array.poi_tree.size(); unload_time++)
    {
        //指向待卸载四叉树
        node = tree_array.poi_tree[unload_time];

        // ---------- 从编号序列中删除 ----------
        int idx = binary_search(tree_info.num_sequence, node->qurdtree_ID,
            greater(), &tree_record<T>::qurdtree_ID);
        tree_info.num_sequence.erase(tree_info.num_sequence.begin() + idx);

        // ---------- 从X坐标序列中删除 ----------
        idx = binary_search(tree_info.X_sequence, node->root.X,
            greater(), [](const tree_record<T>* node) { return node->root.X; });
        tree_info.X_sequence.erase(tree_info.X_sequence.begin() + idx);

        // ---------- 释放资源 ----------
        if (node->tree != nullptr)
        {
            delete node->tree;
            node->tree = nullptr;
        }
        //删除节点本身
        delete node;
    }
}
