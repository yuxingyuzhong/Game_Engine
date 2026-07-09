cmake_minimum_required(VERSION 3.20)

# ==================================================
# 1. 项目基本信息
# ==================================================
project(游戏引擎 LANGUAGES CXX)

# C++ 标准：强制要求 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)   # 禁用编译器扩展，提高可移植性

# ==================================================
# 2. 源文件收集（仅 .cpp，排除构建目录，自动更新）
# ==================================================

# 获取当前 CMake 脚本所在目录的绝对路径
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# 递归收集所有 .cpp 文件，并自动检测新增/删除（CMake >= 3.12）
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_ROOT_DIR}/游戏引擎/*.cpp"
)

# 关键：排除构建目录下的文件，使用构建目录的绝对路径进行安全过滤
if(ENGINE_SOURCES)
    string(REPLACE "\\" "/" SAFE_BINARY_DIR "${CMAKE_BINARY_DIR}")
    string(REGEX REPLACE "([.+*?^$()])" "\\\\\\1" SAFE_BINARY_REGEX "${SAFE_BINARY_DIR}")
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX "^${SAFE_BINARY_REGEX}/")

    # 排除主调文件目录（避免将 main 编译进静态库）
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/主调文件/")
endif()

# 如果没有找到任何 .cpp，报错终止
if(NOT ENGINE_SOURCES)
    message(FATAL_ERROR 
        "未在 '${PROJECT_ROOT_DIR}/游戏引擎/' 下找到任何 .cpp 文件！\n"
        "请检查目录结构是否完整，以及 CMakeLists.txt 是否放置正确。"
    )
endif()

message(STATUS "引擎源文件数量: ${ENGINE_SOURCES}")

# ==================================================
# 3. 创建引擎静态库
# ==================================================
add_library(EngineCore STATIC ${ENGINE_SOURCES})

# ==================================================
# 4. 头文件搜索路径设置
# ==================================================
target_include_directories(EngineCore
    PUBLIC
        "${PROJECT_ROOT_DIR}/游戏引擎"
    PRIVATE
        "${PROJECT_ROOT_DIR}/游戏引擎"
)

# ==================================================
# 5. 第三方库（预留）
# ==================================================

# ==================================================
# 6. 平台兼容性设置
# ==================================================
if(MSVC)
    target_compile_options(EngineCore PRIVATE /MP /utf-8)
    target_compile_definitions(EngineCore PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(EngineCore PRIVATE -Wall -Wextra -pedantic)
    target_compile_definitions(EngineCore PRIVATE _GNU_SOURCE)
endif()

set_target_properties(EngineCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ==================================================
# 7. 安装规则
# ==================================================
install(DIRECTORY "${PROJECT_ROOT_DIR}/游戏引擎/"
    DESTINATION "include/EngineCore"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    PATTERN "Private" EXCLUDE
)
install(TARGETS EngineCore
    ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib"
    RUNTIME DESTINATION "bin"
)

# ==================================================
# 8. 可执行测试目标（用于调试引擎）
# ==================================================
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_ROOT_DIR}/游戏引擎/主调文件/*.cpp"
)

if(TEST_SOURCES)
    add_executable(TestEngine ${TEST_SOURCES})
    target_link_libraries(TestEngine PRIVATE EngineCore)
    target_include_directories(TestEngine PRIVATE "${PROJECT_ROOT_DIR}/游戏引擎")

    if(MSVC)
        target_compile_options(TestEngine PRIVATE /MP /utf-8)
        target_compile_definitions(TestEngine PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # 设置为 VS 的默认启动项目（部分版本可能需要在 VS 内手动再选一次）
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "TestEngine")
else()
    message(WARNING "未找到主调文件目录下的测试源文件，将不会生成可执行文件。")
endif()

# ==================================================
# 9. 预留插件/模块接口
# ==================================================
# add_subdirectory(Plugins/CombatProxy)