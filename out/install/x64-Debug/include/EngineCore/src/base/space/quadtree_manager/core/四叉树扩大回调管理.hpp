#pragma once
#include "src/base/space/quadtree_manager/函数预声明.h"

//四叉树扩大管理_____回调管理四叉树查找
template<typename T>
tree_record<T>* Quadtree_Manager<T>::callback_tree_seek(const coord_double& root)
{
    //简化表示路径
    auto& tree_group = tree_info.ID_sequence;
    //寻找回调管理四叉树
    for (int seek_time = 0; seek_time < tree_group.size(); seek_time++)
    {
        //若根节点坐标一致则为目标树
        if (root.X == tree_group[seek_time]->root.X &&
            root.Y == tree_group[seek_time]->root.Y)
            return tree_group[seek_time];
    }
    //若查找失败则返回空指针
    return nullptr;
}

//四叉树扩大管理_____新四叉树管理范围计算
template<typename T>
void Quadtree_Manager<T>::new_tree_range_calcu(const tree_record<T>* primary_tree, 
    const coord_int& target, coord_range& new_tree)
{

    //设置扫描起点
    //固定在当前树中心简化判断
    //同时伴随一定性能损耗

    //简化表示路径
    auto& min_tree_size = tree_info.min_tree_size;
    //初始化新四叉树X轴左边界
    new_tree.left  = (primary_tree->root.X + 0.5) - min_tree_size;
    //初始化新四叉树X轴右边界
    new_tree.right = new_tree.left + (min_tree_size - 1);
    //初始化新四叉树Y轴上边界
    new_tree.up    = (primary_tree->root.Y - 0.5) + min_tree_size;
    //初始化新四叉树Y轴下边界
    new_tree.down  = new_tree.up - (min_tree_size - 1);

    //计算新四叉树管理范围
    for (;;)
    {
        //若新四叉树左边界在目标右侧
        if (new_tree.left > target.X)
        {
            //更新四叉树边界
            new_tree.left -= min_tree_size;
            new_tree.right -= min_tree_size;
        }
        //若新四叉树右边界在目标左侧
        else if (new_tree.right < target.X)
        {
            //更新四叉树边界
            new_tree.left += min_tree_size;
            new_tree.right += min_tree_size;
        }
        //若新四叉树上边界在目标下侧
        if (new_tree.up < target.Y)
        {
            //更新四叉树边界
            new_tree.up += min_tree_size;
            new_tree.down += min_tree_size;
        }
        //若新四叉树下边界在目标上侧
        else if (new_tree.down > target.Y)
        {
            //更新四叉树边界
            new_tree.up -= min_tree_size;
            new_tree.down -= min_tree_size;
        }

        //若新四叉树包含目标则退出
        if (target.X >= new_tree.left && target.X <= new_tree.right &&
            target.Y >= new_tree.down && target.Y <= new_tree.up)
            break;
    }
}

//四叉树扩大管理方法
template<typename T>
bool Quadtree_Manager<T>::tree_expand_approve(const coord_double& root, const coord_int& target)
{
    //获取当前回调管理四叉树信息
    tree_record<T>* now_tree = nullptr;
    now_tree = this->callback_tree_seek(root);
    //若回调管理四叉树查询失败则直接返回
    if (now_tree == nullptr)
        return false;
    //若四叉树大小未达到上限则进行扩大可行性分析
    else if(now_tree->size < tree_info.max_tree_size)
    {
        //矩形筛选范围存储
        coord_range rectan_range{};
        //当前四叉树扩大区域四叉树根节点存储
        vector<tree_record<T>*> ptr_rectan_tree{};
        //筛选扩大后树管理范围
        //与当前树管理范围的非交集范围(即将管理区域)
        //是否存在其他四叉树根节点
        rectan_range.left = now_tree->root.X - (now_tree->size + now_tree->size) / 2;
        rectan_range.right = now_tree->root.X + (now_tree->size + now_tree->size) / 2;
        rectan_range.up = now_tree->root.Y + (now_tree->size + now_tree->size) / 2;
        rectan_range.down = now_tree->root.Y - (now_tree->size + now_tree->size) / 2;
        //进入矩形筛选
        rectangle_screen(ptr_rectan_tree, now_tree, rectan_range);

        //若不存在其他四叉树根节点
        //则继续筛选扩大区域外
        //检查是否存在四叉树管理范围与扩大区域重叠
        //若有则扩大不可行
        if (ptr_rectan_tree.size() == 0)
        {
            //存储相邻四叉树
            vector<tree_record<T>*> ptr_next_tree{};
            //重定义四叉树大小以满足筛选需要
            now_tree->size *= 2;
            //查找相邻区域四叉树
            next_tree_seek(ptr_next_tree, now_tree);

            //若存在相邻四叉树则筛选重叠四叉树
            if (ptr_next_tree.size() > 0)
            {
                //重置四叉树存储
                vector<tree_record<T>*> ptr_overlap_tree{};
                //重定义筛选方式确保筛选出重叠四叉树
                auto screen_method = [](float coord_1, float coord_2, float coord_3, float coord_4) -> bool
                    {
                        if (coord_1 > coord_2 - 1 && coord_3 > coord_4 - 1)
                            return true;
                        else
                            return false;
                    };
                //确认筛选——筛选重叠四叉树
                next_tree_verify(ptr_overlap_tree, now_tree, ptr_next_tree, screen_method);

                //恢复四叉树真实大小记录
                now_tree->size /= 2;
                //若不存在重叠四叉树则扩大可行
                if (ptr_overlap_tree.size() == 0)
                {
                    //若满足条件则重置当前最大四叉树记录
                    if (now_tree->size * 2 > largest_tree_size)
                        largest_tree_size = now_tree->size * 2;
                    return true;
                }
            }
            //若不存在相邻四叉树则扩大可行
            else
            {
                //恢复四叉树真实大小记录
                now_tree->size /= 2;
                //若满足条件则重置当前最大四叉树记录
                if (now_tree->size * 2 > largest_tree_size)
                    largest_tree_size = now_tree->size * 2;
                return true;
            }
        }
    }

    //————若执行至此则构建新四叉树————//
    
    //新四叉树根节点存储
    coord_double new_root = { 0.5,0.5 };
    //新四叉树管理范围存储
    coord_range new_tree_range{};
    //获取新四叉树管理范围
    new_tree_range_calcu(now_tree, target, new_tree_range);
    //计算新四叉树根节点位置
    new_root.X = (new_tree_range.left + new_tree_range.right) / 2;
    new_root.Y = (new_tree_range.up + new_tree_range.down) / 2;

    //简化表示
    auto& tree_group = tree_info.ID_sequence;
    //寻找待创建四叉树是否已经存在
    for (int seek_time = 0; seek_time < tree_group.size(); seek_time++)
    {
        //若存在则将其移动到编号序列末尾
        if (tree_group[seek_time]->root.X == new_root.X &&
            tree_group[seek_time]->root.Y == new_root.Y)
        {
            //移动目标可查询四叉树到序列末尾
            //完成句柄更新前置工作
            rotate(tree_group.begin() + seek_time, tree_group.begin() + seek_time + 1,
                tree_group.end());
            //返回扩大不可行
            return false;
        }
    }

    //若不存在则创建新四叉树
    qurdtree_build(new_root);
    //返回扩大不可行
    return false;
}
