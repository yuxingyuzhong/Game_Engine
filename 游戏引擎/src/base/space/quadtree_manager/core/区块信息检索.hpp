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
    handle->ptr_tree = tree_info.ID_sequence.back();
    //更新四叉树编号
    handle->ID = tree_info.ID_sequence.back()->quadtree_ID;
    //因为在四叉树扩大管理中已经调整过编号序列
    //故此处需要调整回去
    //对ID_sequence按编号降序排序
    sort(tree_info.ID_sequence.begin(), tree_info.ID_sequence.end(),
        greater(), &tree_record<T>::quadtree_ID);
}

//查询范围列表修改
template<typename T>
void Quadtree_Manager<T>::target_range_amaed(const coord_range& excel_range, bool* ptr_excel,
    const coord_range& target_range)
{
    //简化表示路径
    auto& block_size = tree_info.block_size;

    //X轴起点列索引（包含）
    int X_start = (target_range.left - excel_range.left) / block_size;
    //X轴终点列索引（包含），+1用于循环 < 结束
    int X_end = (target_range.right - excel_range.left) / block_size + 1;
    //Y轴起始行索引（包含），从excel_range.up向下，行号递增
    int Y_start = (excel_range.up - target_range.up) / block_size; // 注意：up在坐标中较大，向下减小
    //Y轴终点行索引（包含）
    int Y_end = (excel_range.up - target_range.down) / block_size + 1;
    //行宽度（每行列数）
    int width = (excel_range.right - excel_range.left + 1) / block_size;

    //开始修改元素
    for (int col = X_start; col < X_end; col++)
    {
        for (int row = Y_start; row < Y_end; row++)
        {
            ptr_excel[row * width + col] = true;
        }
    }
}

//查询结果列表元素坐标化
template<typename T>
void Quadtree_Manager<T>::excel_element_to_coord(const coord_range& excel_range, const int& element_ID,
    coord_int& receiver)
{
    //表格宽度（列数）
    int width = (excel_range.right - excel_range.left + 1) / tree_info.block_size;
    //计算当前元素所在行列
    int row = element_ID / width;
    int col = element_ID % width;
    //将行列转换为坐标（区块左边界和上边界）
    receiver.X = excel_range.left + col * tree_info.block_size;
    receiver.Y = excel_range.up - row * tree_info.block_size;
}

//单区块信息查询
template<typename T>
void Quadtree_Manager<T>::block_info_seek(tree_manager_handle<T>*& handle, coord_int target)
{
    //若当前四叉树序列为空则创建默认四叉树
    if (tree_info.ID_sequence.size() == 0)
        qurdtree_build();

    //若外界传入句柄为空指针
    //则为其分配内存
    if (handle == nullptr)
    {
        //分配内存
        handle = new tree_manager_handle<T>;
        //若内存分配失败则直接返回
        if (handle == nullptr)
            return;
    }

    //简化表示路径
    auto& tree_record = handle->ptr_tree;
    auto& block_data  = handle->ptr_data;
    //若句柄未携带有效四叉树记录
    if (tree_record == nullptr)
    {
        //查找当前句柄直属四叉树
        tree_record = direct_qurdtree_seek(target);
        //若查找直属四叉树失败
        //则启用四叉树序列
        if (tree_record == nullptr)
            tree_record = tree_info.ID_sequence.front();
    }

    cout << tree_info.ID_sequence.size() << endl;
    cout << target;

    //查找申请访问区块信息
    tree_record->tree->block_seek(block_data, target);
    //记录查询四叉树ID
    handle->ID = tree_record->quadtree_ID;

    //若四叉树记录查询过程中失效
    if (block_data == nullptr)
    {
        cout << tree_info.ID_sequence.size() << endl;
        coord_range receiver;
        auto& tree = tree_info.ID_sequence.back();
        tree->tree->manage_range_calcu(receiver,tree->root,tree->size);
        cout << receiver;

        //更新查询句柄
        seek_handle_update(handle);
        //查找申请访问区块信息
        tree_record->tree->block_seek(block_data, target);
    }

}

//范围区块信息查询
template<typename T>
void Quadtree_Manager<T>::block_info_seek(vector<tree_manager_handle<T>>& receiver, 
    const coord_range& target_range)
{
    //简化表示路径
    auto& baseline_tree = tree_info.ID_sequence.front()->tree;
    auto& root = tree_info.ID_sequence.front()->root;
    //可查询范围存储
    coord_range seekable_range = target_range;
    //可查询范围格式化
    baseline_tree->target_range_format(seekable_range,root,tree_info.block_size);
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
    for (int seek_time = 0; seek_time < total_num; seek_time++)
    {
        //若检测到未查找区块
        if (target_excel[seek_time] == false)
        {
            //索引格式化坐标
            excel_element_to_coord(seekable_range, seek_time, target);
            //查找直属四叉树
            tree_record<T>* ptr_tree = direct_qurdtree_seek(target);
            //若未查询到直属四叉树
            if (ptr_tree == nullptr)
            {
                //智能创建合适四叉树
                qurdtree_create_smart({ target });
                //提取新创建四叉树
                ptr_tree = tree_info.ID_sequence.back();
            }
            //查询待查询区块
            ptr_tree->tree->range_seek(seek_result, seekable_range);
            //包装四叉树查询结果
            for (int wrap_index = 0; wrap_index < seek_result.size(); wrap_index++)
                //记录查询结果
                receiver.emplace_back(ptr_tree, ptr_tree->quadtree_ID, seek_result[wrap_index]);
            //获取四叉树管理范围
            ptr_tree->tree->manage_range_calcu(tree_range, ptr_tree->root, ptr_tree->size);
            //裁剪实际查找范围
            ptr_tree->tree->seekable_range_calcu(target_range, tree_range);
            //重置查询结果存储器
            seek_result.clear();
            //修改查询结果列表
            target_range_amaed(seekable_range, target_excel, tree_range);
        }
    }

    //释放指针内存
    delete[] target_excel;
}
