#pragma once
#include "common/前置头文件包含.h"

//游戏引擎命名空间
namespace engine
{
	//实体基类
	//抽象基类不可创建
	class Entity
	{
	protected:
		//实体类型标签
		std::string type{};
		//实体编号
		int64_t ID = 0;
		//实体存活标记
		bool alive = true;
	public:
		Entity() = default;
		virtual ~Entity() = 0;
		//ID信息获取
		int64_t ID_get(void)
		{
			return ID;
		}
		//标签信息获取
		std::string type_get(void)
		{
			return type;
		}
		//存活状态获取
		bool is_alive(void)
		{
			return alive;
		}
	};

	//析构函数定义
	inline Entity::~Entity()
	{

	};
}
