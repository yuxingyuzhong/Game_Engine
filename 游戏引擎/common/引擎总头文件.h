#pragma once

//公共头文件
#include "前置头文件包含.h"

// ———— 基础部分 ————

//空间查询模块
#include "src/base/space/quadtree/四叉树.h"
#include "src/base/space/quadtree_manager/四叉树管理器.h"

//实体模块
#include "src/core/entity/Dynamic_Entity/动态实体.h"
#include "src/core/entity/Entity_Manager/实体管理器.h"

//事件模块
#include "src/core/event/Event_Broker/事件中转器.h"

//工具模块
#include "src/base/tools/Auxi_Algorithm/算法辅助工具.h"
#include "src/base/tools/Config_Loader/配置加载器.h"
#include "src/base/tools/Random/随机数生成器.h"

//脚本模块
#include "src/core/Script_System/EffectComponent/效应组件.h"
#include "src/core/Script_System/SkillWrapComponent/技能参数打包组件.h"

