#pragma once
#include "src/base/space/quadtree_manager/函数预声明.h"

//四叉树扩大管理_____回调管理四叉树查找
template<typename T>
tree_record<T>* Quadtree_Manager<T>::callback_tree_seek(coord_double root)
{
    auto& poi = tree_info.num_sequence;
    for (int seek_time = 0; seek_time < poi.size(); seek_time++)
    {
        if (root.X == poi[seek_time]->root.X &&
            root.Y == poi[seek_time]->root.Y)
            return poi[seek_time];
    }
    //若每找到则返回空指针
    return nullptr;
}

//四叉树扩大管理_____新四叉树管理范围计算
template<typename T>
void Quadtree_Manager<T>::new_tree_range_calcu(const tree_record<T>* primary_tree, const coord_int& seek, coord_range& new_tree)
{
    //X轴扫描方式记录
    screen_way X_screen;
    //Y轴扫描方式记录
    screen_way Y_screen;

    //设置扫描起点
    //固定在当前树中心简化判断
    //同时伴随一定性能损耗

    //初始化新四叉树X轴左边界
    new_tree.left = primary_tree->root.X - 127.5;
    //初始化新四叉树X轴右边界
    new_tree.right = new_tree.left + 255;
    //初始化新四叉树Y轴上边界
    new_tree.up = primary_tree->root.Y + 127.5;
    //初始化新四叉树Y轴下边界
    new_tree.down = new_tree.up - 255;

    //若待查找位置在新四叉树树左侧
    if (seek.X < new_tree.left)
    {
        //设置X轴向左扫描
        X_screen = LEFT_SCREEN;
    }
    //若待查找位置在新四叉树右侧
    else if (seek.X > new_tree.right)
    {
        //设置X轴向右扫描
        X_screen = RIGHT_SCREEN;
    }
    //若待查找位置在新四叉树上侧
    if (seek.Y > new_tree.up)
    {
        //设置Y轴向上扫描
        Y_screen = RISE_SCREEN;
    }
    //若待查找位置在新四叉树下侧
    else if (seek.Y < new_tree.down)
    {
        //设置Y轴向下扫描
        Y_screen = DOWN_SCREEN;
    }

    //检测新四叉树管理范围
    for (;;)
    {
        if (seek.Y >= new_tree.down && seek.Y <= new_tree.up &&
            seek.X <= new_tree.right && seek.X >= new_tree.left)
            break;
        else if (seek.Y <= new_tree.up && seek.Y >= new_tree.down)
        {
            if (X_screen == LEFT_SCREEN)
            {
                //更新四叉树边界
                new_tree.left -= 256;
                new_tree.right -= 256;
            }
            else if (X_screen == RIGHT_SCREEN)
            {
                //更新四叉树边界
                new_tree.left += 256;
                new_tree.right += 256;
            }
        }
        else if (seek.X >= new_tree.left && seek.X <= new_tree.right)
        {
            if (Y_screen == DOWN_SCREEN)
            {
                //更新四叉树边界
                new_tree.up -= 256;
                new_tree.down -= 256;
            }
            else if (Y_screen == RISE_SCREEN)
            {
                //更新四叉树边界
                new_tree.up += 256;
                new_tree.down += 256;
            }
        }
        else
        {
            if (X_screen == LEFT_SCREEN)
            {
                //更新四叉树边界
                new_tree.left -= 256;
                new_tree.right -= 256;
            }
            else if (X_screen == RIGHT_SCREEN)
            {
                //更新四叉树边界
                new_tree.left += 256;
                new_tree.right += 256;
            }
            if (Y_screen == DOWN_SCREEN)
            {
                //更新四叉树边界
                new_tree.up -= 256;
                new_tree.down -= 256;
            }
            else if (Y_screen == RISE_SCREEN)
            {
                //更新四叉树边界
                new_tree.up += 256;
                new_tree.down += 256;
            }
        }
    }
}

//四叉树扩大管理_____新四叉树创建位置计算
template<typename T>
void Quadtree_Manager<T>::create_pos_calcu(const coord_range& new_tree, coord_double& root)
{
    root.X = (new_tree.left + new_tree.right) / 2;
    root.Y = (new_tree.up + new_tree.down) / 2;
}

//四叉树扩大管理方法
template<typename T>
bool Quadtree_Manager<T>::tree_largen_manage(const coord_double& root, const coord_int& seek)
{
    //新四叉树管理范围存储
    coord_range range_new_tree{};
    //新四叉树根节点存储
    coord_double new_root = { 0.5,0.5 };
    //获取当前回调管理四叉树信息
    tree_record<T>* poi_now_tree = nullptr;
    poi_now_tree = this->callback_tree_seek(root);

    //当前四叉树扩大区域四叉树根节点存储
    vector<tree_record<T>*> poi_rectan_tree{};
    //筛选扩大后树管理范围
    //与当前树管理范围的非交集范围(即将管理区域)
    //是否存在其他四叉树根节点
    //获取矩形筛选范围
    rectangle_screen_range_calcu(poi_now_tree, range_new_tree,
        poi_now_tree->size, poi_now_tree->size);
    //进入矩形筛选
    next_tree_seek_rectangle_screen(poi_rectan_tree, poi_now_tree, range_new_tree);

    //若不存在其他四叉树根节点
    //则继续筛选扩大区域外
    //是否存在四叉树管理范围与扩大区域重叠
    //若有则扩大不可行
    //反之扩大可行
    if (poi_rectan_tree.size() == 0)
    {
        //存储相邻四叉树
        vector<tree_record<T>*> poi_next_tree{};
        //重定义四叉树大小以满足筛选需要
        poi_now_tree->size *= 2;
        //查找相邻区域四叉树
        next_tree_seek(poi_next_tree, poi_now_tree);

        //若存在相邻四叉树则筛选重叠四叉树
        if (poi_next_tree.size() > 0)
        {
            //重置四叉树存储
            vector<tree_record<T>*> poi_overlap_tree{};
            //重定义筛选方式确保筛选出重叠四叉树
            auto screen_way = [](float coord_1, float coord_2, float coord_3, float coord_4) -> bool
                {
                    if (coord_1 > coord_2 - 1 && coord_3 > coord_4 - 1)
                        return true;
                    else
                        return false;
                };

            //三级筛选——筛选重叠四叉树
            next_tree_seek_verify(poi_overlap_tree, poi_now_tree, poi_next_tree, screen_way);

            //若存在重叠四叉树则扩大不可行
            if (poi_overlap_tree.size() > 0)
            {
                //恢复四叉树真实大小记录
                poi_now_tree->size /= 2;
                //根据当前四叉树根节点坐标偏移
                //获取新四叉树管理范围
                new_tree_range_calcu(poi_now_tree, seek, range_new_tree);
                //计算新四叉树根节点位置
                create_pos_calcu(range_new_tree, new_root);

                //寻找待创建四叉树是否已经存在
                for (int seek_time = 0; seek_time < tree_info.num_sequence.size(); seek_time++)
                {
                    //简化表示
                    auto& tree_group = tree_info.num_sequence;
                    //若存在则将其移动到编号序列末尾
                    if (tree_group[seek_time]->root.X == new_root.X &&
                        tree_group[seek_time]->root.Y == new_root.Y)
                    {
                        rotate(tree_group.begin() + seek_time, tree_group.begin() + seek_time + 1,
                            tree_group.end());
                        //返回扩大不可行
                        return false;
                    }
                }

                //若不存在则创建新四叉树
                qurdtree_build(new_root);
                return false;
            }
            //若不存在重叠四叉树则扩大可行
            else
            {
                //恢复四叉树真实大小记录
                poi_now_tree->size /= 2;
                //若满足条件则重置当前最大四叉树记录
                if (poi_now_tree->size * 2 > tree_size_largest_now)
                    tree_size_largest_now = poi_now_tree->size * 2;
                return true;
            }
        }
        //若不存在相邻四叉树则扩大可行
        else
        {
            //恢复四叉树真实大小记录
            poi_now_tree->size /= 2;
            //若满足条件则重置当前最大四叉树记录
            if (poi_now_tree->size * 2 > tree_size_largest_now)
                tree_size_largest_now = poi_now_tree->size * 2;
            return true;
        }
    }
    //若存在其他四叉树根节点则扩大不可行
    else
    {
        //获取新四叉树管理范围
        new_tree_range_calcu(poi_now_tree, seek, range_new_tree);
        //计算新四叉树根节点位置
        create_pos_calcu(range_new_tree, new_root);

        //寻找待创建四叉树是否已经存在
        for (int seek_time = 0; seek_time < tree_info.num_sequence.size(); seek_time++)
        {
            //简化表示
            auto& tree_group = tree_info.num_sequence;
            //若存在则将其移动到编号序列末尾
            if (tree_group[seek_time]->root.X == new_root.X &&
                tree_group[seek_time]->root.Y == new_root.Y)
            {
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
}
