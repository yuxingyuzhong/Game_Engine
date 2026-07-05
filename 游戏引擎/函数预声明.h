#pragma once
#include "前置头文件包含.h"
#include "公共命名空间使用.h"
#include "四叉树.h"
#include "算法辅助工具.h"

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
    private:
        //扫描方式结构体
        enum screen_way
        {
            X_SCREEN,
            Y_SCREEN,
            LEFT_SCREEN,
            RIGHT_SCREEN,
            RISE_SCREEN,
            DOWN_SCREEN
        };
        //当前最大四叉树大小
        inline static uint64_t tree_size_largest_now = 256;
        //当前四叉树创建总数
        inline static uint64_t tree_build_total = 1;
    private:
        //四叉树信息
        struct qurdtree_info
        {
            //四叉树序列(依据四叉树编号）
            vector<tree_record<T>*> num_sequence{};
            //四叉树序列(依据根节点X轴坐标)
            vector<tree_record<T>*> X_sequence{};
            //最小区块单元大小
            uint16_t block_size = 16;
            //初始上限默认为65536
            uint64_t max_tree_size = 65536;
        }tree_info;
        //外界上级管理对象回调管理方法_____数据迁移方法
        function<void(tree_block_data<T>& receiver, tree_block_data<T>& transmiter)> copy;
        //外界上级管理对象回调管理方法_____树合并汇报
        function<void(merge_feedback<T>& info)> callback;
        //四叉树范围查询临时中转器

    private:


        //——————相邻四叉树筛选模块——————//

        //四叉树管理范围计算
        void border_qurdtree_calcu(coord_range& receiver, const tree_record<T>* tree);
        //直属四叉树查找
        tree_record<T>* direct_qurdtree_seek(coord_int seek, vector<tree_record<T>*> seekable_set = {});
        //相邻四叉树查找_____矩形筛选范围计算
        void rectangle_screen_range_calcu(const tree_record<T>* tree, coord_range& range,
            const uint16_t& size_1, const uint16_t& size_2);
        //相邻四叉树查找_____矩形筛选
        void next_tree_seek_rectangle_screen(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
            const coord_range& range);
        //相邻四叉树查找_____分类筛选
        void next_tree_seek_classify(vector<tree_record<T>*>& reciver, const tree_record<T>* tree,
            const vector<tree_record<T>*>& candidate);
        //相邻四叉树查找_____验证确认
        template<typename Screen>
        void next_tree_seek_verify(vector<tree_record<T>*>& reciver, const tree_record<T>* tree,
            const vector<tree_record<T>*>& candidate, Screen way);
        //相邻四叉树查找总函数
        void next_tree_seek(vector<tree_record<T>*>& reciver, const tree_record<T>* tree);

        //———————四叉树扩大模块———————//

        //四叉树扩大管理_____回调管理四叉树查找
        tree_record<T>* callback_tree_seek(coord_double root);
        //四叉树扩大管理_____新四叉树管理范围计算
        void new_tree_range_calcu(const tree_record<T>* primary_tree, const coord_int& seek, coord_range& new_tree);
        //四叉树扩大管理_____新四叉树创建位置计算
        void create_pos_calcu(const coord_range& new_tree, coord_double& root);
        //四叉树扩大管理方法
        bool tree_largen_manage(const coord_double& root, const coord_int& seek);

        //——————四叉树合并模块——————//

        //四叉树创建
        void qurdtree_build(coord_double root, uint16_t tree_size = 256);
        //四叉树合并_____收集候选组合
        void quedtree_merge_collect(vector<vector<tree_record<T>*>>& receiver);
        //四叉树合并_____精确筛选组合
        void quedtree_merge_filter(const vector<vector<tree_record<T>*>>& candidate,
            vector<vector<tree_record<T>*>>& receiver);
    public:
        //四叉树合并总函数
        void qurdtree_merge(void);
    private:
        //四叉树卸载
        void qurdtree_unload(merge_feedback<T>& tree_array);

        //——————基础设置模块——————//

    public:
        //上层管理方法注册
        void callback_sign(const function<void(tree_block_data<T>& receiver, tree_block_data<T>& transmiter)>& cb_1,
            const function<void(merge_feedback<T>& info)>& cb_2);
    private:
        //四叉树智能创建——计算初始包围矩形及最大区块划分参数
        void prepare_smart_create_params(const vector<coord_int>& coord_set,
            coord_range& recta_range, int& father_block_num_all,
            int& father_block_size) const;
        //四叉树智能创建——单个最大区块的深度划分（递归复制子集版，保持原接口）
        void divide_single_father_block(const vector<coord_int>& coord_set,
            int block_left, int block_right, int block_up, int block_down,
            int block_size, int coord_count_in_parent,
            vector<coord_double>& node_centers,
            vector<uint64_t>& tree_sizes) const;
    public:
        //四叉树智能创建主函数
        void qurdtree_create_smart(const vector<coord_int>& coord_set);
        //四叉树最小区块单元大小设置
        void set_block_size(const int& size);
        //四叉树边长上限设置
        void set_max_size(const int& size);

        //——————信息查询模块——————//

    private:
        //查询句柄更新
        void seek_handle_update(tree_manager_handle<T>* handle);
        //待查询范围格式化
        void target_range_format(coord_range& target_range);
        //已查询范围计算
        void seeked_range_tailor(const coord_range& target_range, coord_range& tree_range);
        //查询范围列表修改
        void target_range_amaed(const coord_range& excel_range, bool* ptr_excel,
            const coord_range& target_range);
        //查询结果列表元素坐标化
        void excel_element_to_coord(const coord_range& excel_range, const int& element_ID,
            coord_int& receiver);
    public:
        //单区块信息查询
        void block_info_seek(tree_manager_handle<T>*& handle, coord_int target);
        //范围区块信息查询
        void block_info_seek(vector<tree_manager_handle<T>>& receiver, const coord_range& target_range);
    public:
        //坐标四叉树归属问询
        bool coord_inclusion_get(const coord_int& coord, uint16_t& ID);
        //坐标范围四叉树归属数量问询
        bool range_inclusion_get(const coord_range& range, const uint16_t& ID);
        //四叉树相邻树问询
        void next_tree_get(const uint16_t& direct_ID, vector<uint16_t>& other_ID);
        //四叉树数量问询
        void tree_num_get(int& num);
    };
}

//使用四叉树管理器
using engine::Quadtree_Manager;

