#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "src/base/space/quadtree/四叉树.h"
#include "src/base/tools/算法辅助工具.h"

//使用四叉树类
using engine::Quadtree;
//使用四叉树节点结构体
using engine::tree_record;
//使用四叉树输出结构体
using engine::tree_block_data;
//使用四叉树管理器输出结构体
using engine::tree_manager_handle;
//使用四叉树合并/卸载信息结构体
using engine::merge_feedback;

namespace Game_Engine
{
    //四叉树管理器
    template<typename T>
    class Quadtree_Manager
    {
    // ======================== 公开接口 ========================
    public:
        // ---- 设置 ----
        //上层管理方法注册
        void callback_sign(const function<void(tree_block_data<T>& receiver, tree_block_data<T>& transmiter)>& cb_1,
            const function<void(merge_feedback<T>& info)>& cb_2);
        //四叉树最小区块单元大小设置
        void set_block_size(const uint64_t& block_size);
        //四叉树边长上限设置
        void set_max_size(const uint64_t& tree_size);
        //四叉树边长下限设置
        void set_min_size(const uint64_t& tree_size);

        // ---- 创建 ----
        //四叉树智能创建主函数
        void qurdtree_create_smart(const vector<coord_int>& coord_set);

        // ---- 查询 ----
        //单区块信息查询
        void block_info_seek(tree_manager_handle<T>*& handle, coord_int target);
        //范围区块信息查询
        void block_info_seek(vector<tree_manager_handle<T>>& receiver, const coord_range& target_range);
        //坐标四叉树归属获取
        bool coord_inclusion_get(const coord_int& coord, uint16_t& ID);
        //坐标范围四叉树归属数量获取
        bool range_inclusion_get(const coord_range& range, const uint16_t& ID);
        //四叉树相邻树获取
        void next_tree_get(const uint16_t& direct_ID, vector<uint16_t>& other_ID);
        //四叉树数量获取
        int tree_num_get(void);

        // ---- 维护 ----
        //四叉树合并总函数
        void qurdtree_merge(void);

    // ======================== 私有成员 ========================
    private:
        // ---- 内部类型 ----
        //扫描方式结构体
        enum screen_way
        {
            X_SCREEN,
            Y_SCREEN,
        };

        // ---- 数据成员 ----
        inline static uint64_t largest_tree_size = 256;  //当前最大四叉树大小
        inline static uint64_t tree_build_total = 1;         //当前四叉树创建总数

        //四叉树信息
        struct qurdtree_info
        {
            vector<tree_record<T>*> ID_sequence{};      //四叉树序列(依据四叉树编号）
            vector<tree_record<T>*> X_sequence{};       //四叉树序列(依据根节点X轴坐标)
            uint64_t block_size = 16;                   //最小区块单元大小
            uint64_t max_tree_size = 65536;             //初始上限默认为65536
            uint64_t min_tree_size = 256;               //初始下限默认为256
        }tree_info;

        //外界上级管理对象回调管理方法
        function<void(tree_block_data<T>& receiver, tree_block_data<T>& transmiter)> copy;  //数据迁移方法
        function<void(merge_feedback<T>& info)> callback;                                   //树合并汇报

        // ---- 辅助函数（按功能分组） ----

        // ---------- 相邻四叉树筛选 ----------
        //直属四叉树查找
        tree_record<T>* direct_qurdtree_seek(const coord_int& target);
        //矩形筛选
        void rectangle_screen(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
            const coord_range& range);
        //分类筛选
        vector<tree_record<T>*> next_tree_classify(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
            const vector<tree_record<T>*>& candidate);
        //验证确认（模板）
        template<typename Screen>
        void next_tree_verify(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
            const vector<tree_record<T>*>& candidate, Screen way);
        //候选树筛选
        void filter_candidate_tree(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
            const vector<tree_record<T>*>& candidate);
        //相邻四叉树查找总函数
        void next_tree_seek(vector<tree_record<T>*>& receiver, const tree_record<T>* tree);

        // ---------- 四叉树扩大管理 ----------
        //回调管理四叉树查找
        tree_record<T>* callback_tree_seek(const coord_double& root);
        //新四叉树管理范围计算
        void new_tree_range_calcu(const tree_record<T>* primary_tree, const coord_int& seek, coord_range& new_tree);
        //扩大管理方法
        bool tree_expand_approve(const coord_double& root, const coord_int& seek);

        // ---------- 四叉树合并 / 卸载 ----------
        //四叉树创建
        void qurdtree_build(coord_double root = {0.5,0.5}, uint64_t tree_size = 256);
        //合并：收集候选组合
        void quedtree_merge_collect(vector<vector<tree_record<T>*>>& receiver);
        //合并：精确筛选组合
        void quedtree_merge_filter(const vector<vector<tree_record<T>*>>& candidate,
            vector<vector<tree_record<T>*>>& receiver);
        //卸载
        void qurdtree_unload(merge_feedback<T>& tree_array);

        // ---------- 智能创建辅助 ----------
        //计算初始包围矩形及最大区块划分参数
        void prepare_smart_create_params(const vector<coord_int>& coord_set,
            coord_range& recta_range, int& father_block_num_all,
            int& father_block_size) const;
        //单个最大区块的深度划分（递归复制子集版）
        void divide_single_father_block(const vector<coord_int>& coord_set,
            int block_left, int block_right, int block_up, int block_down,
            int block_size, int coord_count_in_parent,
            vector<coord_double>& node_centers,
            vector<uint64_t>& tree_sizes) const;

        // ---------- 查询辅助 ----------
        //查询句柄更新
        void seek_handle_update(tree_manager_handle<T>* handle);
        //查询范围列表修改
        void target_range_amaed(const coord_range& excel_range, bool* ptr_excel,
            const coord_range& target_range);
        //查询结果列表元素坐标化
        void excel_element_to_coord(const coord_range& excel_range, const int& element_ID,
            coord_int& receiver);
    };
}

//使用四叉树管理器
using engine::Quadtree_Manager;