
#include "common/引擎总头文件.h"

//使用功能测试器
using engine::Tester;

int main(void)
{
	// 输出切到 UTF-8（调试输出仍用控制台）
	SetConsoleOutputCP(CP_UTF8);
	// 输入切到 UTF-8
	SetConsoleCP(CP_UTF8);

	Tester tester;
	//进行四叉树单点查找测试
	//tester.qurdtree_block_seek_test(256, 128, 1024, 16, { 0.5,0.5 }, 500);
	//进行四叉树范围查找测试
	//tester.qurdtree_range_seek_test(256, 512, 512, 16, { 0.5,0.5 }, 100);
	//进行四叉树管理器智能创建合并测试
	//tester.manager_tree_build_merge_test(256,1024,128,16384,16,500);
	//进行四叉树管理器单点查找测试
	tester.manager_block_seek_test(256, 1024, 128, 1024, 16, { 0.5,0.5 }, 500);
	//进行四叉树管理器范围查找测试
	//tester.manager_range_seek_test(256, 1024, 128, 1024, 16, { 0.5,0.5 }, 100);
}
