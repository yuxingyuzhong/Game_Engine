#pragma once
#include "src/base/space/quadtree_manager/函数预声明.h"

//查询句柄更新
template<typename T>
void Quadtree_Manager<T>::seek_handle_update(tree_manager_handle<T>* handle)
{
    //当调用该函数时
    //编号序列已经被manage_way函数处理过
    //无论新建四叉树
    //还是在原有四叉树中搜索可用四叉树
    //皆将其置于编号序列末尾
    //故更新时直接从末尾获取树信息即可
    //而若末尾树为原有树
    //则还需重排序编号序列

    //更新四叉树指针
    handle->ptr_tree = tree_info.num_sequence.back();
    //更新四叉树编号
    handle->ID = tree_info.num_sequence.back()->qurdtree_ID;
    //因为在四叉树扩大管理中已经调整过编号序列
    //故此处需要调整回去
    //对num_sequence按编号降序排序
    sort(tree_info.num_sequence.begin(), tree_info.num_sequence.end(),
        greater(), &tree_record<T>::qurdtree_ID);
}

//待查询范围格式化
template<typename T>
void Quadtree_Manager<T>::target_range_format(coord_range& target_range)
{
    //简化表示路径
    auto& root = tree_info.num_sequence.front()->root;
    auto& block_size = tree_info.block_size;

    //格式化目的:保证待查询范围严格包含内部区块

    //格式化左边界
    if ((abs((int)(target_range.left - root.X - 0.5)) % block_size) != 0)
        target_range.left -= block_size - (abs((int)(target_range.left - root.X - 0.5)) % block_size);
    //格式化右边界
    if ((abs((int)(target_range.right - root.X + 0.5)) % block_size) != 0)
        target_range.right += block_size - (abs((int)(target_range.right - root.X + 0.5)) % block_size);
    //格式化上边界
    if ((abs((int)(target_range.up - root.Y + 0.5)) % block_size) != 0)
        target_range.up += block_size - (abs((int)(target_range.up - root.Y + 0.5)) % block_size);
    //格式化下边界
    if ((abs((int)(target_range.down - root.Y - 0.5)) % block_size) != 0)
        target_range.down -= block_size - (abs((int)(target_range.down - root.Y - 0.5)) % block_size);
}

//已查询范围计算
template<typename T>
void Quadtree_Manager<T>::seeked_range_tailor(const coord_range& target_range, coord_range& tree_range)
{
    //若四叉树范围左边界超出查找范围
    if (target_range.left >= tree_range.left)
        tree_range.left = target_range.left;

    //若四叉树范围右边界超出查找范围
    if (target_range.right >= tree_range.right)
        tree_range.right = target_range.right;

    //若四叉树范围上边界超出查找范围
    if (target_range.up >= tree_range.up)
        tree_range.up = target_range.up;

    //若四叉树范围下边界超出查找范围
    if (target_range.down >= tree_range.down)
        tree_range.down = target_range.down;
}

//查询范围列表修改
template<typename T>
void Quadtree_Manager<T>::target_range_amaed(const coord_range& excel_range, bool* ptr_excel,
    const coord_range& target_range)
{
    //简化表示路径
    auto& block_size = tree_info.block_size;

    //X轴起点计算
    int X_start = (target_range.left - excel_range.left) / block_size;
    //X轴终点计算
    int X_end = X_start + ((target_range.right - target_range.left) / block_size);
    //Y轴起点计算
    int Y_start = (target_range.up - excel_range.up) / block_size;
    //Y轴终点计算
    int Y_end = Y_start + (abs(target_range.down - target_range.up) / block_size);
    //行宽度计算
    int width = (excel_range.right - excel_range.left + 1) / block_size;
    //开始修改元素
    for (int column_index = X_start; column_index < X_end; column_index++)
    {
        for (int row_index = Y_start; row_index < Y_end; row_index++)
        {
            ptr_excel[row_index * width + column_index] = true;
        }
    }
}

//查询结果列表元素坐标化
template<typename T>
void Quadtree_Manager<T>::excel_element_to_coord(const coord_range& excel_range, const int& element_ID,
    coord_int& receiver)
{
    //表格宽度
    int width = (excel_range.right - excel_range.left + 1) / tree_info.block_size;
    //表格高度
    int height = (excel_range.up - excel_range.down + 1) / tree_info.block_size;
    //格式化接收器
    receiver.X = excel_range.right;
    receiver.Y = excel_range.up;
    //剩余序号存储
    int remain_ID = element_ID;
    //简化表示路径
    auto& block_size = tree_info.block_size;

    for (int seek_time = 0; seek_time < height; seek_time++)
    {
        //计算剩余序号
        remain_ID = element_ID - seek_time * width;

        //若剩余序号超出本行
        if (remain_ID > width)
            receiver.Y -= block_size;
        //若剩余序号不足本行
        else if (remain_ID < width)
            receiver.X - (width - remain_ID) * block_size;
    }
}

//单区块信息查询
template<typename T>
void Quadtree_Manager<T>::block_info_seek(tree_manager_handle<T>*& handle, coord_int target)
        {
            //若外界传入句柄为空指针
            //则为其分配内存
            if (handle == nullptr)
            {
                //分配内存
                handle = new tree_manager_handle<T>;
                //若内存分配失败则直接返回
                if (handle == nullptr)
                {
                    return;
                }
            }

            //简化表示路径
            auto& tree_node = handle->ptr_tree;
            auto& block_data = (handle->ptr_data);
            if (tree_node == nullptr)
            {
                //查找当前句柄直属四叉树
                tree_node = direct_qurdtree_seek(target);
                //若查找直属四叉树失败
                //则直接启用四叉树序列
                if (tree_node == nullptr)
                    tree_node = tree_info.num_sequence.front();
                //查找申请访问区块信息
                tree_node->tree->block_seek(block_data, target);
                //记录查询四叉树ID
                handle->ID = tree_node->qurdtree_ID;
            }
            else if (tree_node != nullptr)
            {
                //查找申请访问区块信息
                tree_node->tree->block_seek(block_data, target);
                if (block_data == nullptr)
                {
                    //更新查询句柄
                    seek_handle_update(handle);
                    //查找申请访问区块信息
                    tree_node->tree->block_seek(block_data, target);
                }
            }
        }

//范围区块信息查询
template<typename T>
void Quadtree_Manager<T>::block_info_seek(vector<tree_manager_handle<T>>& receiver, const coord_range& target_range)
        {
            //可查询范围计算
            coord_range seekable_range = target_range;
            target_range_format(seekable_range);
            //待查询区块数计算
            int total_num = ((seekable_range.right - seekable_range.left + 1) / tree_info.block_size) *
                ((seekable_range.up - seekable_range.down + 1) / tree_info.block_size);

            //分配足量内存存储查询结果列表
            bool* target_excel = new(nothrow) bool[total_num]();
            //若分配失败则直接返回
            if (target_excel == nullptr)
                return;

            //查找坐标存储
            coord_int target{};
            //四叉树管理范围存储
            coord_range tree_range{};
            //四叉树返回结果存储
            vector<tree_block_data<T>*> seek_result{};

            //内层循环查找结果
            for (int seek_index = 0; seek_index < total_num; seek_index++)
            {
                //若检测到未查找区块
                if (target_excel[seek_index] == false)
                {
                    //索引格式化坐标
                    excel_element_to_coord(seekable_range, seek_index, target);
                    //查找直属四叉树
                    tree_record<T>* ptr_tree = direct_qurdtree_seek(target);
                    //若未查询到直属四叉树
                    if (ptr_tree == nullptr)
                    {
                        //智能创建合适四叉树
                        qurdtree_create_smart({ target });
                        //重新查找
                        ptr_tree = direct_qurdtree_seek(target);
                    }
                    //查询待查询区块
                    ptr_tree->tree->range_seek(seek_result, seekable_range);
                    //包装四叉树查询结果
                    for (int wrap_index = 0; wrap_index < seek_result.size(); wrap_index++)
                    {
                        //记录查询结果
                        receiver.emplace_back(ptr_tree, ptr_tree->qurdtree_ID, seek_result[wrap_index]);
                    }
                    //获取四叉树管理范围
                    border_qurdtree_calcu(tree_range, ptr_tree);
                    //裁剪实际查找范围
                    seeked_range_tailor(target_range, tree_range);
                    //重置查询结果存储器
                    seek_result.clear();
                    //修改查询结果列表
                    target_range_amaed(seekable_range, target_excel, tree_range);
                }
            }

            //释放指针内存
            delete[] target_excel;
        }
