#pragma once
#include "src/core/space/Quadtree_Manager/函数预声明.h"

//引擎命名空间
namespace engine
{
    //四叉树扩大管理_____回调管理四叉树查找
    template<typename T>
    tree_record<T>* Quadtree_Manager<T>::callback_tree_seek(const coord2D_double& root)
    {
        //简化表示路径
        auto& tree_group = X_sequence;
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
    void Quadtree_Manager<T>::new_tree_range_calcu(const tree_record<T>* baseline_tree,
        const coord2D_int& target, coord2D_range& new_tree)
    {
        //简化表示路径
        auto& min_tree_size = settings.min_tree_size;
        auto& root = baseline_tree->root;
        //计算新四叉树起始范围
        baseline_tree->tree->manage_range_calcu(new_tree, root, min_tree_size);

        //若基准四叉树大小不为初始大小
        //则对起始范围进行进一步校准
        if (baseline_tree->size != settings.min_tree_size)
        {
            //X轴偏移量
            uint64_t offset_x = min_tree_size / 2;
            //Y轴偏移量
            uint64_t offset_y = min_tree_size / 2;

            //若目标X轴坐标小于根节点坐标
            if (target.X < root.X)
                offset_x *= -1;
            //若目标Y轴坐标小于根节点坐标
            if (target.Y < root.Y)
                offset_y *= -1;

            //校准范围
            new_tree.left += offset_x;
            new_tree.right += offset_x;
            new_tree.up += offset_y;
            new_tree.down += offset_y;
        }

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
    bool Quadtree_Manager<T>::tree_expand_approve(const coord2D_double& root, const coord2D_int& target, bool internal)
    {
        //获取当前回调管理四叉树信息
        tree_record<T>* now_tree = nullptr;
        now_tree = this->callback_tree_seek(root);
        //若回调管理四叉树查询失败则直接返回
        if (now_tree == nullptr)
            return false;

        //若目标点直属四叉树查找成功则直接返回
        if (quadtree_inclusion_seek(target) != nullptr)
            return false;
        //若目标点直属四叉树查找失败
        //且回调管理四叉树大小未达到上限则进行扩大可行性分析
        else if (now_tree->size < settings.max_tree_size)
        {
            //矩形筛选范围存储
            coord2D_range rectan_range{};
            //当前四叉树扩大区域四叉树根节点存储
            std::vector<tree_record<T>*> ptr_rectan_tree{};
            //筛选扩大后树管理范围
            //与当前树管理范围的非交集范围(即将管理区域)
            //是否存在其他四叉树根节点
            rectan_range.left = now_tree->root.X - (now_tree->size + now_tree->size) / 2;
            rectan_range.right = now_tree->root.X + (now_tree->size + now_tree->size) / 2;
            rectan_range.up = now_tree->root.Y + (now_tree->size + now_tree->size) / 2;
            rectan_range.down = now_tree->root.Y - (now_tree->size + now_tree->size) / 2;
            //进入矩形筛选
            rectangle_filter(ptr_rectan_tree, now_tree, rectan_range);

            //若不存在其他四叉树根节点
            //则继续筛选扩大区域外
            //检查是否存在四叉树管理范围与扩大区域重叠
            //若有则扩大不可行
            if (ptr_rectan_tree.size() == 0)
            {
                //存储相邻四叉树
                std::vector<tree_record<T>*> next_trees{};
                //重定义四叉树大小以满足筛选需要
                now_tree->size *= 2;
                //查找相邻区域四叉树
                next_tree_seek(next_trees, now_tree);

                //若存在相邻四叉树则筛选重叠四叉树
                if (next_trees.size() > 0)
                {
                    //重置四叉树存储
                    std::vector<tree_record<T>*> ptr_overlap_tree{};
                    //重定义筛选方式确保筛选出重叠四叉树
                    auto screen_method = [](float coord_1, float coord_2, float coord_3, float coord_4) -> bool
                        {
                            if (coord_1 > coord_2 - 1 && coord_3 > coord_4 - 1)
                                return true;
                            else
                                return false;
                        };
                    //确认筛选——筛选重叠四叉树
                    next_tree_verify(ptr_overlap_tree, now_tree, next_trees, screen_method);

                    //恢复四叉树真实大小记录
                    now_tree->size /= 2;
                    //若不存在重叠四叉树则扩大可行
                    if (ptr_overlap_tree.size() == 0)
                    {
                        //更新四叉树大小记录
                        now_tree->size *= 2;
                        //若满足条件则重置当前最大四叉树记录
                        if (now_tree->size > largest_tree_size)
                            largest_tree_size = now_tree->size;
                        //若当前函数为内部调用
                        if (internal == true)
                            now_tree->tree->tree_expand();

                        //简化表示路径
                        auto& records = tree_cache.records;
                        auto& ranges = tree_cache.ranges;
                        //检查缓存是否失效
                        for (int exam_time = 0; exam_time < records.size(); exam_time++)
                        {
                            //若缓存失效则更新缓存
                            if (now_tree == records[exam_time])
                                now_tree->tree->manage_range_calcu(ranges[exam_time],
                                    now_tree->root, now_tree->size);
                        }

                        return true;
                    }
                }
                //若不存在相邻四叉树则扩大可行
                else
                {
                    //若满足条件则重置当前最大四叉树记录
                    if (now_tree->size > largest_tree_size)
                        largest_tree_size = now_tree->size;
                    //若当前函数为内部调用
                    if (internal == true)
                        now_tree->tree->tree_expand();

                    //简化表示路径
                    auto& records = tree_cache.records;
                    auto& ranges = tree_cache.ranges;
                    //检查缓存是否失效
                    for (int exam_time = 0; exam_time < records.size(); exam_time++)
                    {
                        //若缓存失效则更新缓存
                        if (now_tree == records[exam_time])
                            now_tree->tree->manage_range_calcu(ranges[exam_time],
                                now_tree->root, now_tree->size);
                    }
                    return true;
                }
            }
        }

        //————若执行至此则构建新四叉树————//

        //新四叉树根节点存储
        coord2D_double new_root = { 0.5,0.5 };
        //新四叉树管理范围存储
        coord2D_range new_tree_range{};
        //获取新四叉树管理范围
        new_tree_range_calcu(now_tree, target, new_tree_range);
        //计算新四叉树根节点位置
        new_root.X = (new_tree_range.left + new_tree_range.right) / 2.0f;
        new_root.Y = (new_tree_range.up + new_tree_range.down) / 2.0f;

        //创建新四叉树
        quadtree_build(new_root, settings.min_tree_size);

        //返回扩大不可行
        return false;
    }

}

