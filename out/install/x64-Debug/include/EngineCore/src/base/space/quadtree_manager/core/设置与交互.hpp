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
void Quadtree_Manager<T>::set_block_size(const uint64_t& block_size)
{
    //记录最小区块单元信息
    tree_info.block_size = block_size;
    //设置四叉树最小区块单元信息
    auto& tree_group = tree_info.ID_sequence;
    for (int set_time = 0; set_time < tree_group.size(); set_time++)
    {
        tree_group[set_time]->tree->set_block_size(tree_info.block_size);
    }
}

//四叉树边长上限设置
template<typename T>
void Quadtree_Manager<T>::set_max_size(const uint64_t& tree_size)
{
    //记录四叉树上限大小信息
    tree_info.max_tree_size = tree_size;
    //设置四叉树上限大小信息
    auto& tree_group = tree_info.ID_sequence;
    for (int set_time = 0; set_time < tree_group.size(); set_time++)
    {
        tree_group[set_time]->tree->set_max_size(tree_info.max_tree_size);;
    }
}

//四叉树边长下限设置
template<typename T>
void Quadtree_Manager<T>::set_min_size(const uint64_t& tree_size)
{
    //记录四叉树下限大小信息
    tree_info.min_tree_size = tree_size;
    //设置四叉树下限大小信息
    auto& tree_group = tree_info.ID_sequence;
    for (int set_time = 0; set_time < tree_group.size(); set_time++)
    {
        tree_group[set_time]->tree->set_max_size(tree_info.min_tree_size);;
    }
}

//坐标四叉树归属获取
template<typename T>
bool Quadtree_Manager<T>::coord_inclusion_get(const coord_int& coord, uint16_t& ID)
{
    //查找其直属四叉树
    tree_record<T>* tree = direct_qurdtree_seek(coord);
    //若存在该四叉树则返回true
    if (tree != nullptr)
    {
        ID = tree->quadtree_ID;
        return true;
    }
    //若不存在则返回false
    else
        return false;
}

//坐标范围四叉树归属数量获取
template<typename T>
bool Quadtree_Manager<T>::range_inclusion_get(const coord_range& range, const uint16_t& ID)
{
    //临时构造结构封装ID信息（对象）
    tree_record<T> temp_record{ nullptr, {}, 256, ID };
    tree_record<T>* target = &temp_record;

    //简化表示路径
    auto& tree_group = tree_info.ID_sequence;
    //查找索引信息
    int index = binary_search(tree_group.begin(), tree_group.end(),
        target->quadtree_ID, greater(), &tree_record<T>::quadtree_ID);
    //若索引查找失败则直接返回false
    if (index == -1)
        return false;

    //简化表示路径
    auto& ptr_tree = tree_group[index];
    //计算索引对应四叉树管理范围
    coord_range tree_range{};
    ptr_tree->tree->manage_range_calcu(tree_range, ptr_tree.root, ptr_tree.size);

    //检测当前四叉树是否完全覆盖参数坐标范围
    //若是则该参数坐标范围归属单棵四叉树
    if (tree_range.left <= range.left &&
        tree_range.right >= range.right &&
        tree_range.down <= range.down &&
        tree_range.up >= range.up)
        return true;
    //若否则归属多棵
    else
        return false;
}

//四叉树相邻树获取
template<typename T>
void Quadtree_Manager<T>::next_tree_get(const uint16_t& direct_ID, vector<uint16_t>& other_ID)
{
    //构造包含直属四叉树ID信息的临时对象
    tree_record<T> temp_record{ nullptr, {}, 256, direct_ID };
    tree_record<T>* target = &temp_record;

    //简化表示路径
    auto& tree_group = tree_info.ID_sequence;
    //二分查找该ID对应四叉树
    int index = binary_search(tree_group.begin(), tree_group.end(),
        target->qurdtree_ID, greater(), &tree_record<T>::qurdtree_ID);
    //若未找到直属四叉树则直接返回
    if (index == -1)
        return;
    
    //若查找成功则获取该记录完整信息
    target = tree_group[index];
    //相邻四叉树查找结果存储
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
        //记录相应编号
        for (int record_time = 0; record_time < receiver.size(); record_time++)
            other_ID.push_back(receiver[record_time]->qurdtree_ID);
    }
}

//四叉树数量获取
template<typename T>
int Quadtree_Manager<T>::tree_num_get(void)
{
    return tree_info.ID_sequence.size();
}
