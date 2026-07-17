#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "src/base/space/quadtree/四叉树.h"
#include "src/base/tools/算法辅助工具.h"

//使用四叉树类
using engine::Quadtree;
//使用四叉树节点结构体
using engine::tree_record;
//使用四叉树管理器设置结构体
using engine::tree_manager_settings;
//使用四叉树输出结构体
using engine::tree_block_data;

namespace Game_Engine
{
    //四叉树管理器
    template<typename T>
    class Quadtree_Manager
    {
    // ======================== 公开接口 ========================
    public:
        //默认构造函数
        Quadtree_Manager();
        //默认析构函数
        ~Quadtree_Manager();
            
        // ---- 设置 ----
        //数据迁移方法注册
        void callback_sign(const function<void(tree_block_data<T>& receiver, 
            tree_block_data<T>& transmiter)>& cb_1);
        //四叉树最小区块单元大小设置
        void set_block_size(const uint64_t& block_size);
        //四叉树边长上限设置
        void set_max_size(const uint64_t& max_size);
        //四叉树边长下限设置
        void set_min_size(const uint64_t& min_size);
        //高速缓存启用状态设置
        void set_cache_state(bool enabled);
        //高速缓存启用阈值设置
        void set_cache_active_threshold(const uint64_t& threshold);
        //高速缓存条目上限设置
        void set_max_cach_records(const uint64_t max_entries);

        // ---- 创建 ----
        //四叉树智能创建主函数
        void qurdtree_build_smart(const vector<coord_int>& coord_set);

        // ---- 查询 ----
        //单区块信息查询
        void seek(tree_block_data<T>*& reciver,const coord_int& target, bool stable);
        //范围区块信息查询
        void seek(vector<tree_block_data<T>*>& receiver, const coord_range& target_range, bool stable);

        // ---- 读取 ----
        //四叉树管理器设置获取
        const tree_manager_settings& settings_get(void);
        //四叉树序列档案信息获取
        const vector<tree_record<T>*>& records_get(void);
        //最大四叉树大小获取
        const uint64_t& largest_size_get(void);

        // ---- 维护 ----
        //四叉树合并总函数
        void qurdtree_merge(void);
        //清空高速缓存
        void cache_clear(void);
        //清空所有四叉树
        void clear(void);
        //卸载——根节点重载
        void quadtree_unload(const vector<coord_double>& root_set);


    // ======================== 私有成员 ========================
    private:
        // ---- 数据成员 ----
        
        //当前最大四叉树大小
        uint64_t largest_tree_size = 0;  
        //四叉树序列(依据根节点X轴坐标降序)
        vector<tree_record<T>*> X_sequence{};
        //四叉树高速缓存
        struct
        {
            //缓存记录
            vector<tree_record<T>*> records{};
            //缓存范围
            vector<coord_range> ranges{};
        }tree_cache;
        //管理器设置
        tree_manager_settings settings;
        //外界上级管理对象回调管理方法
        function<void(tree_block_data<T>& receiver, tree_block_data<T>& transmiter)> copy;  //数据迁移方法

        // ---- 辅助函数（按功能分组） ----

        // ---------- 相邻四叉树筛选 ----------
        //直属四叉树查找
        tree_record<T>* quadtree_inclusion_seek(const coord_int& target);
        //四叉树序列索引查找
        int64_t quadtree_index_seek(const coord_double& root);
        //矩形筛选
        void rectangle_filter(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
            const coord_range& range, const vector<tree_record<T>*>* tree_group = nullptr);
        //分类筛选
        vector<tree_record<T>*> next_tree_classify(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
            const vector<tree_record<T>*>& candidate);
        //验证确认（模板）
        template<typename Screen>
        void next_tree_verify(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
            const vector<tree_record<T>*>& candidate, Screen way);
        //候选树筛选
        void candidate_tree_filter(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
            const vector<tree_record<T>*>& candidate);
        //相邻四叉树查找总函数
        void next_tree_seek(vector<tree_record<T>*>& receiver, const tree_record<T>* tree, const vector<tree_record<T>*>* tree_group = nullptr);

        // ---------- 四叉树扩大管理 ----------
        //回调管理四叉树查找
        tree_record<T>* callback_tree_seek(const coord_double& root);
        //新四叉树管理范围计算
        void new_tree_range_calcu(const tree_record<T>* primary_tree, const coord_int& seek, coord_range& new_tree);
        //扩大管理方法
        bool tree_expand_approve(const coord_double& root, const coord_int& seek, bool internal = false);

        // ---------- 四叉树合并 / 卸载 ----------
        //四叉树创建
        void quadtree_build(coord_double root = {0.5,0.5}, uint64_t tree_size = 256);
        //合并：收集候选组合
        void quedtree_merge_collect(vector<vector<tree_record<T>*>>& receiver);
        //合并：精确筛选组合
        void quedtree_merge_filter(const vector<vector<tree_record<T>*>>& candidate,
            vector<vector<tree_record<T>*>>& receiver);
        //卸载——序列索引重载
        void quadtree_unload(vector<int64_t>& index_set);

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