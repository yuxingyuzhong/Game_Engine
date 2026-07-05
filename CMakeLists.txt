cmake_minimum_required(VERSION 3.20)

# 项目名称（可使用中文，仅用于显示）
project(游戏引擎 LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动收集“游戏引擎”子目录下所有源文件
file(GLOB_RECURSE ENGINE_SOURCES 
    "游戏引擎/*.cpp" 
    "游戏引擎/*.h" 
    "游戏引擎/*.hpp"
)

# 过滤掉编译输出目录 x64 和 out 中的残留文件
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/(x64|out)/.*")

# 调试输出：查看找到的文件列表（可在 VS 输出窗口查看）
message(STATUS "找到的源文件: ${ENGINE_SOURCES}")

# 如果没找到任何源文件则终止并报错
if(NOT ENGINE_SOURCES)
    message(FATAL_ERROR "未找到任何源文件！请检查 '游戏引擎/' 目录是否存在，以及路径是否正确。")
endif()

# 创建引擎核心静态库（目标名必须为英文）
add_library(EngineCore STATIC ${ENGINE_SOURCES})

# 设置公共包含路径：所有源文件都可以从“游戏引擎”目录开始写 include
target_include_directories(EngineCore PUBLIC "游戏引擎")

# 获取并打印 EngineCore 目标当前的公共包含路径
get_target_property(CURRENT_INCLUDES EngineCore INCLUDE_DIRECTORIES)
message(STATUS "EngineCore 实际的公共包含路径: ${CURRENT_INCLUDES}")
      
# ===== 以后添加插件或独立模块的预留接口 =====
# add_subdirectory(Plugins/CombatProxy)