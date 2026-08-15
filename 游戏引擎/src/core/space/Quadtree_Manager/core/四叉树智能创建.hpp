#pragma once
#include "../函数预声明.h"

//引擎命名空间
namespace engine
{
    //四叉树智能创建——计算初始包围矩形及最大区块划分参数
    template<typename T>
    void Quadtree_Manager<T>::prepare_smart_create_params(const std::vector<coord2D_int>& coord_set,
        coord2D_range& recta_range, int& father_block_num_all,
        int& father_block_size) const
    {
        //初始化包围矩形
        recta_range.left = coord_set.front().X;
        recta_range.right = coord_set.front().X;
        recta_range.up = coord_set.front().Y;
        recta_range.down = coord_set.front().Y;

        //矩形边界确认
        for (int time = 0; time < coord_set.size(); time++)
        {
            //若坐标在矩形左边界外
            if (recta_range.left > coord_set[time].X)
                recta_range.left = coord_set[time].X;
            //若坐标在矩形右边界外
            if (recta_range.right < coord_set[time].X)
                recta_range.right = coord_set[time].X;
            //若坐标在矩形上边界外
            if (recta_range.up < coord_set[time].Y)
                recta_range.up = coord_set[time].Y;
            //若坐标在矩形下边界外
            if (recta_range.down > coord_set[time].Y)
                recta_range.down = coord_set[time].Y;
        }

        //确定最大区块边长（位移量）
        father_block_size = settings.max_tree_size;

        //计算矩形宽度
        int width = (recta_range.right - recta_range.left + 1);
        //若边界未对齐则补齐边界
        if (width % father_block_size != 0)
        {
            recta_range.right += father_block_size - (width % father_block_size);
            width += father_block_size - (width % father_block_size);
        }
        //计算矩形包含最大区块数目（X轴）
        int father_block_num_X = width / father_block_size;

        //计算矩形高度
        int height = (recta_range.up - recta_range.down + 1);
        //若边界未对齐则补齐边界
        if (height % father_block_size != 0)
        {
            recta_range.up += father_block_size - (height % father_block_size);
            height += father_block_size - (height % father_block_size);
        }
        // 计算矩形包含最大区块数目（Y轴）
        int father_block_num_Y = height / father_block_size;

        // 更新总最大区块个数
        father_block_num_all = father_block_num_X * father_block_num_Y;
    }

    //四叉树智能创建——单个最大区块的深度划分（递归复制子集版，保持原接口）
    template<typename T>
    void Quadtree_Manager<T>::divide_single_father_block(const std::vector<coord2D_int>& coord_set,
        int block_left, int block_right, int block_up, int block_down,
        int block_size, int coord_count_in_parent,
        std::vector<coord2D_double>& node_centers,
        std::vector<uint64_t>& tree_sizes) const
    {
        // 辅助：判断点是否在当前区块内
        auto in_block = [&](const coord2D_int& p) -> bool {
            return p.X >= block_left && p.X <= block_right &&
                p.Y >= block_down && p.Y <= block_up;
            };

        // 收集当前区块内的所有点（复制子集）
        std::vector<coord2D_int> local_points;
        for (const auto& p : coord_set)
            if (in_block(p))
                local_points.push_back(p);

        // 没有点，直接返回（不建树）
        if (local_points.empty())
            return;

        // 最小区块（256）：直接建树
        if (block_size == 256)
        {
            float cx = (block_left + block_right) / 2.0f;
            float cy = (block_down + block_up) / 2.0f;
            node_centers.push_back({ cx, cy });
            tree_sizes.push_back(block_size);
            return;
        }

        // 检查当前区块内的所有点是否中心聚集
        float center_x = (block_left + block_right) / 2.0f;
        float center_y = (block_down + block_up) / 2.0f;
        float limit = 0.375f * block_size;
        bool centered = true;
        for (const auto& p : local_points)
        {
            if (std::fabs(p.X - center_x) > limit || std::fabs(p.Y - center_y) > limit)
            {
                centered = false;
                break;
            }
        }

        // 如果不中心聚集，在当前区块建树
        if (!centered)
        {
            float cx = (block_left + block_right) / 2.0f;
            float cy = (block_down + block_up) / 2.0f;
            node_centers.push_back({ cx, cy });
            tree_sizes.push_back(block_size);
            return;
        }

        // 中心聚集 -> 尝试细分
        int half = block_size / 2;
        // 四个子区块边界
        struct SubRect { int l, r, d, u; };
        SubRect subs[4] = {
            { block_left, block_left + half - 1, block_down + half, block_up },           // NW
            { block_left + half, block_right,     block_down + half, block_up },          // NE
            { block_left, block_left + half - 1, block_down, block_down + half - 1 },      // SW
            { block_left + half, block_right,     block_down, block_down + half - 1 }       // SE
        };

        // 将 local_points 分配到四个子区块
        std::vector<coord2D_int> sub_points[4];
        for (const auto& p : local_points)
        {
            if (p.X <= block_left + half - 1)   // 左半
            {
                if (p.Y >= block_down + half)   // 上半
                    sub_points[0].push_back(p);
                else                            // 下半
                    sub_points[2].push_back(p);
            }
            else                                // 右半
            {
                if (p.Y >= block_down + half)   // 上半
                    sub_points[1].push_back(p);
                else                            // 下半
                    sub_points[3].push_back(p);
            }
        }

        // 统计非空子区块数量
        int nonempty = 0;
        int target = -1;
        for (int i = 0; i < 4; ++i)
        {
            if (!sub_points[i].empty())
            {
                nonempty++;
                target = i;
            }
        }

        if (nonempty == 1)
        {
            // 点全部落在同一个子区块 -> 递归细分该子区块
            const auto& sub = subs[target];
            divide_single_father_block(sub_points[target],
                sub.l, sub.r, sub.u, sub.d,   // 注意顺序：l,r,u,d
                half, sub_points[target].size(),
                node_centers, tree_sizes);
        }
        else
        {
            // 点分散到多个子区块 -> 在当前区块建树
            float cx = (block_left + block_right) / 2.0f;
            float cy = (block_down + block_up) / 2.0f;
            node_centers.push_back({ cx, cy });
            tree_sizes.push_back(block_size);
        }
    }

    //四叉树智能创建主函数
    template<typename T>
    void Quadtree_Manager<T>::qurdtree_build_smart(const std::vector<coord2D_int>& coord_set)
    {
        // 智能创建逻辑：
        // 先用一个初始矩形包裹住所有坐标点
        // 然后将矩形分割为若干块最大四叉树所管理的矩形
        // 之后不断子级分割
        // 直至保证子区块尽可能小，且里面的点尽可能多
        // 尽可能远离区块边界

        //简化表示路径
        auto& tree_group = X_sequence;

        //若四叉树集合尚未存在
        if (tree_group.size() == 0)
        {
            // —————————— 第一步：准备包围矩形和最大区块参数 ——————————

            //点集分布范围存储
            coord2D_range recta_range{};
            //最大区块数量
            int max_block_num_total = 0;
            //最大区块边长
            int max_block_size = 0;
            //计算矩形范围和最大区块参数
            prepare_smart_create_params(coord_set, recta_range, max_block_num_total, max_block_size);

            // —————————— 第二步：遍历点集划分可递归区块 ——————————

            //各区块内坐标个数记录
            std::vector<int> father_block_coord_count(max_block_num_total, 0);

            //访问索引记录
            int index = 0;
            //中转坐标存储
            coord2D_int middle_store{};
            //水平竖直方向包含区块数目计算
            int father_block_num_X = (recta_range.right - recta_range.left + 1) / max_block_size;

            //统计各区块包含坐标数
            for (int time = 0; time < coord_set.size(); time++)
            {
                //计算当前点所在列号（水平方向第几块）
                int col_index = (coord_set[time].X - recta_range.left) / max_block_size;
                //计算当前点所在行号（垂直方向从上往下第几块）
                int row_index = (recta_range.up - coord_set[time].Y) / max_block_size;
                //合成一维区块索引
                int index = row_index * father_block_num_X + col_index;

                //增加相关区块计数器
                father_block_coord_count[index]++;
            }

            // —————————— 第三步：对可递归区块进行划分 ——————————

            //各最大区块管理范围记录
            coord2D_range block_range{};
            //简化表示路径
            auto& left = block_range.left;
            auto& right = block_range.right;
            auto& up = block_range.up;
            auto& down = block_range.down;

            //待创建四叉树树根节点记录
            std::vector<coord2D_double> root{};
            //待创建四叉树大小记录
            std::vector<uint64_t> tree_size{};

            //寻找可划分区块
            for (int find_index = 0; find_index < max_block_num_total; find_index++)
            {
                // 重置区块边界（修正为闭区间）
                left = recta_range.left;
                right = left + max_block_size - 1;   // 修正：减1
                up = recta_range.up;
                down = up - max_block_size + 1;      // 修正：加1

                //对符合条件的区块进行划分
                if (father_block_coord_count[find_index] != 0)
                {
                    //计算区块左边界
                    left += (find_index % father_block_num_X) * max_block_size;
                    //计算区块右边界（闭区间）
                    right = left + max_block_size - 1;   // 修正：减1
                    //计算区块上边界
                    up -= (find_index / father_block_num_X) * max_block_size;
                    //计算区块下边界（闭区间）
                    down = up - max_block_size + 1;      // 修正：加1

                    //调用区块划分函数
                    divide_single_father_block(coord_set, left, right, up, down,
                        max_block_size, father_block_coord_count[find_index],
                        root, tree_size);
                }
            }

            // —————————— 第四步：统一创建所有四叉树 ——————————
            for (int create_time = 0; create_time < root.size(); create_time++)
            {
                quadtree_build(root[create_time], tree_size[create_time]);
            }
        }
        //若四叉树集合已经存在
        else
        {
            //检查点集坐标是否有四叉树覆盖
            for (int exam_time = 0; exam_time < coord_set.size(); exam_time++)
            {
                //若点集坐标尚未被四叉树覆盖
                //则调用扩大管理函数
                //利用其自适应机制完成四叉树的创建
                if (quadtree_inclusion_seek(coord_set[exam_time]) == nullptr)
                    tree_expand_approve(tree_group.back()->root, coord_set[exam_time], true);
            }
        }
    }

}
