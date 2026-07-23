#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"

//游戏引擎命名空间
namespace Game_Engine
{
	//实体基类
	//抽象基类不可创建
	class Entity
	{
	protected:
		//实体类型标签
		string tag{};
		//实体编号
		uint64_t ID = 0;
		//实体存活标记
		bool alive = true;
	public:
		Entity() = default;
		virtual ~Entity() = 0;
	};

}
