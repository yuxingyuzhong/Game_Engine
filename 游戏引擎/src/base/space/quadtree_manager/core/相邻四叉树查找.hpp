#pragma once
#include "src/base/space/quadtree_manager/函数预声明.h"

//相邻四叉树查找_____矩形筛选范围计算
template<typename T>
void Quadtree_Manager<T>::rectangle_screen_range_calcu(const tree_record<T>* tree, coord_range& range,
    const uint16_t& size_1, const uint16_t& size_2)
{
    //矩形筛选时
    //以主体四叉树根节点坐标为中心
    //size_1和size_2之和为边长
    range.left = tree->root.X - (size_1 + size_2) / 2;
    range.right = tree->root.X + (size_1 + size_2) / 2;
    range.down = tree->root.Y - (size_1 + size_2) / 2;
    range.up = tree->root.Y + (size_1 + size_2) / 2;
}

//相邻四叉树查找_____矩形筛选
template<typename T>
void Quadtree_Manager<T>::next_tree_seek_rectangle_screen(vector<tree_record<T>*>& receiver, const tree_record<T>* tree,
    const coord_range& range)
{
    /*/
    矩形筛选逻辑：选定待查找树，记录其根节点坐标，然后以根节点坐标为原点
    待查找树大小一半加全场最大四叉树大小一半为边长进行筛选
        筛选逻辑：
        第一次，以X轴范围为准，将筛选结果放置进一个临时vector
        第二次，以Y轴范围为准，在原来筛选出的临时vector里再次筛选
                直接筛选出根节点坐标在矩形内的四叉树
    /**/

    //临时筛选结果接收
    vector<tree_record<T>*> temp_receiver{};

    //检测X轴坐标
    auto& poi = tree_info.X_sequence;
    for (int screen_time_now = 0; screen_time_now < poi.size(); screen_time_now++)
    {
        //若候选四叉树根节点X坐标位于矩形筛选范围内
        //且与主体四叉树不为同一棵四叉树则满足条件

        //简化表示路径
        auto& now_tree = poi[screen_time_now];
        if (now_tree->root.X >= range.left &&
            now_tree->root.X <= range.right &&
            now_tree->qurdtree_ID != tree->qurdtree_ID)
            temp_receiver.push_back(now_tree);
    }

    //若未筛选出合格四叉树则直接返回
    if (temp_receiver.size() == 0)
        return;

    //待删除元素索引标记
    int back_idex = temp_receiver.size();
    //检测Y轴坐标
    for (int screen_time_now = 0; screen_time_now < temp_receiver.size(); screen_time_now++)
    {
        //在原来筛选基础上再次筛选				
        //若候选四叉树根节点Y坐标位于矩形筛选范围内
        //则将该四叉树放置进接收vector中

        //简化表示路径
        auto& now_tree = temp_receiver[screen_time_now];
        if (now_tree->root.Y >= range.down &&
            now_tree->root.Y <= range.up &&
            now_tree->qurdtree_ID != tree->qurdtree_ID)
            receiver.push_back(now_tree);
    }
}

//相邻四叉树查找_____分类筛选
template<typename T>
void Quadtree_Manager<T>::next_tree_seek_classify(vector<tree_record<T>*>& reciver, const tree_record<T>* tree,
    const vector<tree_record<T>*>& candidate)
{
    /*/
    二级筛选逻辑：
             检查vector内每一个四叉树的根节点X/Y坐标与待查找树根节点X/Y坐标相减绝对值
             是否小于等于两棵四叉树的边长之和的一半。
             若X/Y中一个坐标满足条件，则进入第三级筛选，若X/Y均满足条件则直接记录为相邻树
    /**/

    //等待进入四级筛选四叉树存储
    vector<tree_record<T>*> poi_wait_third_tree{};
    //四级筛选合格四叉树存储
    vector<tree_record<T>*> poi_screen_third_tree{};

    //根节点坐标差值
    coord_int coord_diff = { 0,0 };
    //满足筛选条件次数
    int meet_screen_time = 0;

    for (int screen_time = 0; screen_time < candidate.size(); screen_time++)
    {
        //重置满足筛选条件次数
        meet_screen_time = 0;
        //重置根节点X坐标差值
        coord_diff.X = tree->root.X - candidate[screen_time]->root.X;
        //若差值为负数则取绝对值
        if (coord_diff.X < 0)
            coord_diff.X = -coord_diff.X;
        //重置根节点Y坐标差值
        coord_diff.Y = tree->root.Y - candidate[screen_time]->root.Y;
        //若差值为负数则取绝对值
        if (coord_diff.Y < 0)
            coord_diff.Y = -coord_diff.Y;

        //若X轴坐标差值小于等于两四叉树边长和之一半
        //则满足条件
        if (coord_diff.X <= (tree->size + candidate[screen_time]->size) / 2)
            meet_screen_time++;
        //若Y轴坐标差值小于等于两四叉树边长和之一半
        //则满足条件
        if (coord_diff.Y <= (tree->size + candidate[screen_time]->size) / 2)
            meet_screen_time++;
        //若满足两个条件则必定为对角线相邻树
        //结束其筛选
        if (meet_screen_time == 2)
            reciver.push_back(candidate[screen_time]);
        //若满足一个条件则为主体四叉树周围一排树
        //筛选进入第四级
        else if (meet_screen_time == 1)
            poi_wait_third_tree.push_back(candidate[screen_time]);
    }

    //四叉树筛选方式lambda
    auto screen_way = [](float coord_1, float coord_2, float coord_3, float coord_4) -> bool
        {
            if (coord_1 >= coord_2 - 1 && coord_3 >= coord_4 - 1)
                return true;
            else
                return false;
        };
    //PS:该lambda的筛选逻辑会筛选重叠四叉树
       //目的为四叉树扩大回调管理出进一步锁定重叠四叉树
       //故此处逻辑不需修改

    //三级筛选
    next_tree_seek_verify(poi_screen_third_tree, tree, poi_wait_third_tree, screen_way);

    //将poi_screen_forth_tree的所有元素追加到reciver尾部
    reciver.insert(reciver.end(), poi_screen_third_tree.begin(), poi_screen_third_tree.end());

}

//相邻四叉树查找_____验证确认
template<typename T>
template<typename Screen>
void Quadtree_Manager<T>::next_tree_seek_verify(vector<tree_record<T>*>& reciver, const tree_record<T>* tree,
    const vector<tree_record<T>*>& candidate, Screen way)
{
    /*/
    三级筛选逻辑：
             计算vector内的四叉树的管理范围是否与待查找树存在交集
             若有则为相邻树
    /**/

    //主体四叉树边界存储
    coord_range range_subject_tree{};
    //计算主体四叉树边界
    border_qurdtree_calcu(range_subject_tree, tree);

    //当前候选四叉树边界存储
    coord_range range_now_tree{};

    for (int screen_time = 0; screen_time < candidate.size(); screen_time++)
    {
        //重置候选相邻四叉树边界
        border_qurdtree_calcu(range_now_tree, candidate[screen_time]);

        //重置坐标交集

        //X轴坐标交集
        float left1 = range_subject_tree.left, right1 = range_subject_tree.right;
        float left2 = range_now_tree.left, right2 = range_now_tree.right;
        bool is_x_next = way(right1, left2, right2, left1);

        //Y轴坐标交集
        float down1 = range_subject_tree.down, up1 = range_subject_tree.up;
        float down2 = range_now_tree.down, up2 = range_now_tree.up;
        bool is_y_next = way(up1, down2, up2, down1);

        //当一侧交集不为0即为相邻树
        if (is_x_next == true || is_y_next == true)
        {
            reciver.push_back(candidate[screen_time]);
        }
    }

}

//相邻四叉树查找总函数
template<typename T>
void Quadtree_Manager<T>::next_tree_seek(vector<tree_record<T>*>& reciver, const tree_record<T>* tree)
{
    //矩形四叉树筛选结果存储
    vector<tree_record<T>*> poi_rectan_tree{};
    //筛选以四叉树为中心的矩形范围内是否存在相邻四叉树
    coord_range range{};
    rectangle_screen_range_calcu(tree, range,
        tree->size, tree_size_largest_now);
    next_tree_seek_rectangle_screen
    (poi_rectan_tree, tree, range);
    //若未筛选出合格四叉树或内存分配失败则直接返回
    if (poi_rectan_tree.size() == 0)
    {
        reciver = poi_rectan_tree;
        return;
    }

    //若筛选出合格四叉树
    //二级筛选(及其内部调用三级筛选)
    next_tree_seek_classify(reciver, tree, poi_rectan_tree);

}
