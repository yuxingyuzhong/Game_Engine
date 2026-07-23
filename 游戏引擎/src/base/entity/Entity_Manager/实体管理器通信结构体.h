#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "src/base/event/Event_Broker/事件中转站通信结构体.h"

//游戏引擎命名空间
namespace Game_Engine
{
	//实体管理器派生事件
	struct entity_event : public config_event
	{
		//发起者类型标签
		string sender;
		//发起者实体ID
		uint64_t ID = 0;

		//使用默认等于运算符
		bool operator==(const entity_event&) const = default;
	};

}