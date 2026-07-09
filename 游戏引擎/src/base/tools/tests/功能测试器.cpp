#include "功能测试器.h"
//使用坐标生成器
using engine::Gen_Coord;
//使用测试数据类型
using engine::Test_Data;
//使用测试器
using engine::Tester;

//====四叉树单点查找测试====//
void Tester::qurdtree_block_seek_test(const uint64_t& tree_size_start,
	const int64_t& coord_radius_start, const int64_t& coord_radius_max,
	const uint64_t& block_size, const coord_double& root, uint64_t test_times)
{
	//创建四叉树
	Quadtree<Test_Data>* tree = new Quadtree<Test_Data>(tree_size_start, root);
	//若内存分配失败则直接返回
	if (tree == nullptr)
		return ;
	//设置四叉树最小区块单元
	tree->set_block_size(block_size);

	//测试坐标存储
	vector<coord_int> coord_group;
	//坐标查询结果存储
	vector<tree_block_data<Test_Data>*> handle_group{};
	//坐标查询结果存储
	vector<tree_block_data<Test_Data>*> range_group{};
	//当前测试半径存储
	uint64_t radius_now = coord_radius_start;

	//循环测试四叉树基本功能
	for (; ;)
	{
		cout << format("当前测试半径: {}\n", coord_radius_start);

		//循环查找坐标
		for (int test_time = 0; test_time < test_times; test_time++)
		{
			//生成整数坐标
			coord_group.push_back(gen.gen_coord(-coord_radius_start, coord_radius_start));
			//初始化空指针
			handle_group.push_back(nullptr);
			//查询获取信息
			tree->block_seek(handle_group.back(), coord_group.back());
			//若查询失败
			if (handle_group.back() == nullptr)
			{
				//格式化坐标存储
				coord_double format_coord = coord_group.back();
				//坐标格式化以获得该区块节点坐标
				gen.format_coord(format_coord, root, block_size);
				//输出查询信息
				cout << format("当前根节点坐标: \n");
				cout << root;
				cout << format("查询失败点所属区块坐标: \n");
				cout << format_coord;
			}
			else
			{
				
				//设置数值以待校验
				Test_Data::set_num(*(handle_group.back()), gen(-coord_radius_start, coord_radius_start));
				//继续循环
				continue;
			}
		}

		//循环校验查找结果
		for (int test_time = 0; test_time < test_times; test_time++)
		{
			//检验句柄
			tree_block_data<Test_Data>* examiner = nullptr;
			//重新查询相同坐标
			tree->block_seek(examiner, coord_group[test_time]);
			//若两次查询结果都为空句柄
			if (examiner == nullptr || handle_group[test_time] == nullptr)
			{
				//格式化坐标存储
				coord_double format_coord = coord_group[test_time];
				//坐标格式化以获得该区块节点坐标
				gen.format_coord(format_coord, root, block_size);
				//若两次查询结果均为空句柄
				if (examiner == nullptr && handle_group[test_time] == nullptr)
					cout << format("查询结果异常\n两次均查找失败\n");
				//若只有单次查询结果为空句柄
				else
					cout << format("查询结果异常\n单次查找失败\n");
				//输出查询坐标信息
				cout << format("查询坐标为: \n");
				cout << coord_group[test_time];
				cout << format("节点坐标为: \n");
				cout << format_coord;
				cout << format("请仔细排查BUG\n");
			}
			//若两次查找均成功
			//进行查询结果检验
			else
				Test_Data::compare(*handle_group[test_time], *examiner);
		}

		//重置存储器并且释放内存
		coord_group.clear();
		handle_group.clear();

		//扩大测试半径
		radius_now *= 2;

		//若四叉树半径过大则结束测试
		if (radius_now >= coord_radius_max)
		{
			//测试四叉树析构函数
			delete tree;
			break;
		}
	}

}

//====四叉树范围查找测试====//
void Tester::qurdtree_range_seek_test(const uint64_t& tree_size_start,
	const int64_t& range_radius_start, const int64_t& range_radius_max,
	const uint64_t& block_size, const coord_double& root, uint64_t test_times)
{
	//创建四叉树
	Quadtree<Test_Data>* tree = new Quadtree<Test_Data>(tree_size_start, root);
	//若内存分配失败则直接返回
	if (tree == nullptr)
		return ;
	//设置四叉树最小区块单元
	tree->set_block_size(block_size);

	//测试半径
	int test_radius = range_radius_start;
	//测试范围存储
	coord_range test_range;
	//坐标查询结果存储
	vector<tree_block_data<Test_Data>*> receiver{};

	//循环测试四叉树基本功能
	for (; ;)
	{
		//cout << format("当前测试半径: {}\n", test_radius);

		//循环查找范围
		for (int test_time = 0; test_time < test_times; test_time++)
		{
			//生成查询范围
			test_range = (gen.gen_range(-test_radius, test_radius));
			//输出常规信息
			//cout << format("\n初始查询范围 :\n");
			//cout << test_range << endl;
			//格式化查询范围
			tree->target_range_format(test_range, root, block_size);
			//输出常规信息
			//cout << format("根节点坐标: :\n");
			//cout << root << endl;
			//cout << format("最小区块单元大小 :\n");
			//cout << block_size << endl;
			//cout << format("格式化查询范围 :\n");
			//cout << test_range << endl;

			//预计查询区块数量
			int num_expected = ((test_range.up - test_range.down + 1) / block_size) *
				               ((test_range.right - test_range.left + 1) / block_size);

			//重置范围查询结果
			receiver.clear();
			//查询获取信息
			tree->range_seek(receiver, test_range);
			//若查询获得区块数量与预期不一致 
			if (receiver.size() < num_expected ||
				receiver.size() > num_expected)
				cout << format("查询结果异常\n");
			//若查询获得区块数量和预期一致
			else
				cout << format("查询结果正常\n");
			//输出常规信息
			cout << format("预计区块数量:{} \n",
				num_expected);
			cout << format("实际区块数量: ");
			cout << receiver.size() << endl;
		}
		//输出查询结果
		for (int out_time = 0; out_time < receiver.size(); out_time++)
		{
			//cout << "当前为区块:" << out_time + 1 << endl;
			//Test_Data::out(*receiver[out_time]);
			//cout << endl;
		}

		//扩大测试半径
		test_radius *= 2;
		//若四叉树半径过大则结束测试
		if (test_radius >= range_radius_max)
		{
			//测试四叉树析构函数
			delete tree;
			break;
		}
	}

}

//====四叉树管理器单点查找测试====//
void Tester::manager_block_seek_test(const uint64_t& tree_size_start, const uint64_t& tree_size_max,
	const int64_t& coord_radius_start, const int64_t& coord_radius_max,
	const uint64_t& block_size, const coord_double& root, uint64_t test_times)
{
	//四叉树管理器声明
	Quadtree_Manager<Test_Data> manager{};
	//设置四叉树最大大小
	manager.set_max_size(tree_size_max);
	//设置四叉树最小大小
	manager.set_min_size(256);
	//设置最小区块单元大小
	manager.set_block_size(block_size);
	//创建初始四叉树
	manager.qurdtree_create_smart({ root });
	//测试坐标存储
	vector<coord_int> coord_group;
	//当前测试半径
	int radius_now = coord_radius_start;
	//坐标查询结果存储
	vector<tree_manager_handle<Test_Data>*> handle_group{};
	//循环测试单点查找功能
	for (;;)
	{
		cout << format("当前测试半径: {}\n", radius_now);

		//循环查找坐标
		for (int test_time = 0; test_time < test_times; test_time++)
		{
			//生成整数坐标
			coord_group.push_back(gen.gen_coord(-radius_now, radius_now));
			//初始化空指针
			handle_group.push_back(nullptr);
			//查询获取信息
			manager.block_info_seek(handle_group.back(), coord_group.back());
			//若查询失败
			if (handle_group.back() == nullptr)
			{
				//格式化坐标存储
				coord_double format_coord = coord_group.back();
				//坐标格式化以获得该区块节点坐标
				gen.format_coord(format_coord,root,block_size);
				//输出查询信息
				cout << format("查询失败点所属区块坐标: \n");
				cout << format_coord;
			}
			else
			{
				//设置数值以待校验
				Test_Data::set_num(*(handle_group.back()->ptr_data), gen(-radius_now, radius_now));
				//继续循环
				continue;
			}
		}

		//循环校验查找结果
		for (int test_time = 0; test_time < test_times; test_time++)
		{
			//检验句柄
			tree_manager_handle<Test_Data>* examiner = nullptr;
			//重新查询相同坐标
			manager.block_info_seek(examiner, coord_group[test_time]);
			//若两次查询结果都为空句柄
			if (examiner == nullptr || handle_group[test_time] == nullptr)
			{
				//格式化坐标存储
				coord_double format_coord = coord_group[test_time];
				//坐标格式化以获得该区块节点坐标
				gen.format_coord(format_coord, root, block_size);
				//若两次查询结果均为空句柄
				if (examiner == nullptr && handle_group[test_time] == nullptr)
					cout << format("查询结果异常\n两次均查找失败\n");
				//若只有单次查询结果为空句柄
				else
					cout << format("查询结果异常\n单次查找失败\n");
				//输出查询坐标信息
				cout << format("查询坐标为: \n");
				cout << coord_group[test_time];
				cout << format("节点坐标为: \n");
				cout << format_coord;
				cout << format("请仔细排查BUG\n");
			}
			//若两次查找均成功
			//进行查询结果检验
			else
				Test_Data::compare(*(handle_group[test_time]->ptr_data), *(examiner->ptr_data));
		}

		//重置存储器并且释放内存
		coord_group.clear();
		handle_group.clear();

		//扩大测试半径
		radius_now *= 2;

		//若四叉树半径过大则结束测试
		if (radius_now >= coord_radius_max)
			break;
	}

}

//====四叉树管理器范围查找测试====//
void Tester::manager_range_seek_test(const uint64_t& tree_size_start, const uint64_t& tree_size_max,
	const int64_t& range_radius_start, const int64_t& range_radius_max,
	const uint64_t& block_size, const coord_double& root, uint64_t test_times)
{
	//四叉树管理器声明
	Quadtree_Manager<Test_Data> manager{};
	//设置四叉树最大大小
	manager.set_max_size(tree_size_max);
	//设置最小区块单元大小
	manager.set_block_size(block_size);
	//创建初始四叉树
	manager.qurdtree_create_smart({ root });
	//测试坐标存储
	coord_range range;
	//当前测试半径
	int radius_now = range_radius_start;
	//坐标查询结果存储
	vector<tree_manager_handle<Test_Data>> manager_range_group{};

	for (;;)
	{
		cout << format("当前测试半径: {}\n", radius_now);

		//循环查找范围
		for (int test_time = 0; test_time < test_times; test_time++)
		{
			//重置范围查询结果
			manager_range_group.clear();
			//生成范围坐标
			range = (gen.gen_range(-radius_now, radius_now));
			//格式化范围坐标
			gen.format_range(range, root, block_size);
			//预计查询区块数量
			int block_num_expected = ((range.up - range.down + 1) / block_size) *
				((range.right - range.left + 1) / block_size);
			//查询获取信息
			manager.block_info_seek(manager_range_group, range);
			//若查询获得区块数量与预期不一致 
			if (manager_range_group.size() < block_num_expected ||
				manager_range_group.size() > block_num_expected)
				cout << format("查询结果异常\n");
			//若查询获得区块数量和预期一致
			else
				cout << format("查询结果正常\n");
			//输出常规信息
			cout << format("当前查询范围 :\n");
			cout << range << endl;
			cout << format("预计区块数量:{} \n",
				block_num_expected);
			cout << format("实际区块数量: ");
			cout << manager_range_group.size() << endl;
		}

		//扩大测试半径
		radius_now *= 2;

		//若四叉树半径过大则结束测试
		if (radius_now >= range_radius_max)
			break;
	}
}

//====四叉树创建合并测试====//
void Tester::manager_tree_build_merge_test(const uint64_t& tree_size_start, const uint64_t& tree_size_max,
	const int64_t& coord_radius_start, const int64_t& coord_radius_max,
	const uint64_t& block_size,uint64_t test_times)
{
	//四叉树管理器声明
	Quadtree_Manager<Test_Data> manager{};
	//设置四叉树最大大小
	manager.set_max_size(tree_size_max);
	//设置最小区块单元大小
	manager.set_block_size(block_size);
	//测试坐标存储
	vector<coord_int> coord_group;
	//当前测试半径
	int radius_now = coord_radius_start;
	//设置四叉树合并信息接收回调
	auto callback = [](merge_feedback<Test_Data>& info)
		{
			//输出被合并四叉树信息
			for (int time = 0; time < info.old_tree_ID.size(); time++)
			{
				cout << format("被合并四叉树编号: {}\n", info.old_tree_ID[time]);
			}
			//输出新四叉树信息
			for (int time = 0; time < info.new_tree_ID.size(); time++)
			{
				cout << format("新四叉树编号: {}\n", info.new_tree_ID[time]);
			}
		};
	manager.callback_sign(Test_Data::copy, callback);

	//循环测试智能创建和四叉树合并
	for (;;)
	{
		//重置坐标点集
		coord_group.clear();
		//填充坐标点集
		for (int gen_time = 0; gen_time < test_times; gen_time++)
			//生成整数坐标
			coord_group.push_back(gen.gen_coord(-radius_now, radius_now));
		//进行四叉树智能创建
		manager.qurdtree_create_smart(coord_group);
		//输出四叉树数量
		cout << format("当前四叉树数量: {}\n", manager.tree_num_get());
		//进行四叉树合并
		manager.qurdtree_merge();
		//扩大坐标生成半径
		radius_now *= 2;
		//若坐标生成半径过大则退出
		if (radius_now > coord_radius_max)
			break;
	}
}

