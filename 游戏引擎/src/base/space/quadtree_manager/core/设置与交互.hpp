#pragma once
#include "src/base/space/quadtree_manager/函数预声明.h"

//上层管理方法注册
template<typename T>
void Quadtree_Manager<T>::callback_sign(const function<void(tree_block_data<T>& receiver, tree_block_data<T>& transmiter)>& cb_1,
    const function<void(merge_feedback<T>& info)>& cb_2)
{
    //赋值函数注册
    copy = cb_1;
    //回调管理办法注册
    callback = cb_2;
}

//四叉树最小区块单元大小设置
template<typename T>
void Quadtree_Manager<T>::set_block_size(const int& size)
{
    tree_info.block_size = size;
    for (int set_time = 0; set_time < tree_info.num_sequence.size(); set_time++)
    {
        tree_info.num_sequence[set_time]->tree->set_block_size(tree_info.block_size);
    }
}

//四叉树边长上限设置
template<typename T>
void Quadtree_Manager<T>::set_max_size(const int& size)
{
    tree_info.max_tree_size = size;
    for (int set_time = 0; set_time < tree_info.num_sequence.size(); set_time++)
    {
        tree_info.num_sequence[set_time]->tree->set_max_size(tree_info.max_tree_size);;
    }
}

template<typename T>
bool Quadtree_Manager<T>::coord_inclusion_get(const coord_int& coord, uint16_t& ID)
{
    //查找其直属四叉树
    tree_record<T>* now_tree = direct_qurdtree_seek(coord);
    //若存在该四叉树则返回true
    if (now_tree != nullptr)
    {
        ID = now_tree->qurdtree_ID;
        return true;
    }
    //若不存在则返回false
    else
        return false;
}

//坐标范围四叉树归属数量问询
template<typename T>
bool Quadtree_Manager<T>::range_inclusion_get(const coord_range& range, const uint16_t& ID)
{
    //临时构造结构封装ID信息（对象）
    tree_record<T> temp_node{ nullptr, {}, 256, ID };
    tree_record<T>* target = &temp_node;

    //查找索引信息
    int index = binary_search(tree_info.num_sequence.begin(), tree_info.num_sequence.end(),
        target->qurdtree_ID, greater(), &tree_record<T>::qurdtree_ID);
    //若索引查找失败则直接返回false
    if (index == -1)
    {
        return false;
    }

    // 计算索引对应四叉树管理范围
    coord_range this_tree_range{};
    border_qurdtree_calcu(this_tree_range, tree_info.num_sequence[index]);

    //检测当前四叉树是否完全覆盖参数坐标范围
    //若是则该参数坐标范围归属单棵四叉树
    if (this_tree_range.left <= range.left &&
        this_tree_range.right >= range.right &&
        this_tree_range.down <= range.down &&
        this_tree_range.up >= range.up)
        return true;
    //若否则归属多棵
    else
        return false;
}

//四叉树相邻树问询
template<typename T>
void Quadtree_Manager<T>::next_tree_get(const uint16_t& direct_ID, vector<uint16_t>& other_ID)
{
    //构造包含直属四叉树ID信息的临时对象
    tree_record<T> now_tree{};
    tree_record<T>* target = &now_tree;
    target->qurdtree_ID = direct_ID;
    //二分查找该ID对应四叉树
    int index = binary_search(tree_info.num_sequence.begin(), tree_info.num_sequence.end(),
        target->qurdtree_ID, greater(), &tree_record<T>::qurdtree_ID);
    //若未找到直属四叉树则直接返回
    if (index == -1)
    {
        return;
    }
    else
        target = tree_info.num_sequence[index];

    //临时相邻四叉树查找结果存储
    vector<tree_record<T>*> receiver{};
    //根据直属四叉树寻找相邻四叉树
    next_tree_seek(receiver, target);
    //若筛选结果不为空则进行处理
    if (receiver.size() > 0)
    {
        //将筛选结果按照编号排序
        //对receiver按编号降序排序
        sort(receiver.begin(), receiver.end(),
            greater(), &tree_record<T>::qurdtree_ID);
        //设置相应编号
        for (int time = 0; time < receiver.size(); time++)
            other_ID.push_back(receiver[time]->qurdtree_ID);
    }
}

//四叉树数量问询
template<typename T>
int Quadtree_Manager<T>::tree_num_get(void)
{
    return tree_info.num_sequence.size();
}
