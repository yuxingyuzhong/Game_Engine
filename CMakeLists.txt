# =============================================================================
# CMake 最低版本要求（必须 ≥ 3.20，因为使用了 CONFIGURE_DEPENDS 等特性）
# =============================================================================
cmake_minimum_required(VERSION 3.20)

if(MSVC)
    add_compile_options(/WX-)
endif()

# =============================================================================
# 1. 项目基本信息
# =============================================================================
# 项目名称：游戏引擎（支持中文，但建议英文）
# 语言：C++（我们同时会编译 C 文件，但主语言设定为 CXX 以启用 C++ 标准）
project(游戏引擎 LANGUAGES CXX)

# 强制使用 C++20 标准（Sol2 需要 C++17 以上，这里设 20 更现代）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # 如果编译器不支持 C++20 则报错
set(CMAKE_CXX_EXTENSIONS OFF)         # 禁用 GCC/MSVC 的编译器扩展，提高跨平台兼容性

# =============================================================================
# 2. 源文件收集（关键：同时收集 .cpp 和 .c 文件）
# =============================================================================
# 获取当前 CMakeLists.txt 所在目录的绝对路径（即项目根目录）
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# 递归搜索 "游戏引擎" 子目录下的所有 .cpp 和 .c 文件
# CONFIGURE_DEPENDS 让 CMake 在构建时自动检测新增/删除的源文件（不必手动重新运行 cmake）
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_ROOT_DIR}/游戏引擎/*.cpp"
    "${PROJECT_ROOT_DIR}/游戏引擎/*.c"      # 重要：Lua 官方源码是 .c，必须包含
)

# ----- 过滤掉不需要的文件 -----
if(ENGINE_SOURCES)
    # 1) 排除构建目录下的文件（防止将生成的临时文件误编译进去）
    #    将反斜杠转为正斜杠，方便正则匹配
    string(REPLACE "\\" "/" SAFE_BINARY_DIR "${CMAKE_BINARY_DIR}")
    # 转义正则特殊字符（如 . + * ? 等）
    string(REGEX REPLACE "([.+*?^$()])" "\\\\\\1" SAFE_BINARY_REGEX "${SAFE_BINARY_DIR}")
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX "^${SAFE_BINARY_REGEX}/")

    # 2) 排除 "主调文件" 目录（该目录包含 main()，不应编入静态库）
    list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/主调文件/")
endif()

# 如果过滤后没有源文件，终止配置并给出明确错误提示
if(NOT ENGINE_SOURCES)
    message(FATAL_ERROR 
        "未在 '${PROJECT_ROOT_DIR}/游戏引擎/' 下找到任何 .cpp 或 .c 文件！\n"
        "请检查目录结构是否完整，以及 CMakeLists.txt 是否放置正确。"
    )
endif()

# 打印源文件数量，方便调试
message(STATUS "引擎源文件数量: ${ENGINE_SOURCES}")

# =============================================================================
# 3. 创建引擎静态库（核心）
# =============================================================================
add_library(EngineCore STATIC ${ENGINE_SOURCES})

# ----- 特殊处理：仅将 Lua 源码目录下的文件强制按 C 编译 -----
# 只处理位于 ${LUA_SRC_DIR} 目录下的源文件（无论扩展名是 .c 还是 .cpp）
set(LUA_SOURCES "")
foreach(SOURCE ${ENGINE_SOURCES})
    # 获取源文件所在的目录（统一为 Unix 风格路径，便于比较）
    get_filename_component(SOURCE_DIR "${SOURCE}" DIRECTORY)
    string(REPLACE "\\" "/" SOURCE_DIR "${SOURCE_DIR}")
    string(REPLACE "\\" "/" LUA_DIR_NORM "${LUA_SRC_DIR}")
    # 如果该文件就在 Lua 源码目录下，则加入集合
    if(SOURCE_DIR STREQUAL LUA_DIR_NORM)
        list(APPEND LUA_SOURCES ${SOURCE})
    endif()
endforeach()

if(LUA_SOURCES)
    set_source_files_properties(${LUA_SOURCES} PROPERTIES LANGUAGE C)
    message(STATUS "已强制将 Lua 源码目录下的 ${LUA_SOURCES} 个文件按 C 语言编译")
endif()

# =============================================================================
# 4. 头文件搜索路径设置（使用 PUBLIC 让依赖者继承）
# =============================================================================
# PUBLIC 表示这个包含目录不仅对 EngineCore 自身可见，也对链接 EngineCore 的其他目标（如 TestEngine）可见。
# 这样 TestEngine 无需重复添加同样的路径，减少冗余。
target_include_directories(EngineCore PUBLIC "${PROJECT_ROOT_DIR}/游戏引擎")

# =============================================================================
# 5. 第三方库
# =============================================================================

# ---------- Sol2（纯头文件库） ----------
set(SOL2_ROOT "${PROJECT_ROOT_DIR}/游戏引擎/external/Sol2/include")
target_include_directories(EngineCore PUBLIC "${SOL2_ROOT}")

# ---------- Lua（源码直接编译） ----------
set(LUA_SRC_DIR "${PROJECT_ROOT_DIR}/游戏引擎/external/Lua")
target_include_directories(EngineCore PUBLIC "${LUA_SRC_DIR}")

# ---------- Json（纯头文件库） ----------
set(JSON_ROOT "${PROJECT_ROOT_DIR}/游戏引擎/external/Json")
target_include_directories(EngineCore PUBLIC "${JSON_ROOT}")

# =============================================================================
# 6. 平台兼容性设置（编译选项、宏定义、输出目录）
# =============================================================================
if(MSVC)
    # MSVC 特有选项：
    # /MP   : 多核编译加速
    # /utf-8: 指定源文件字符集为 UTF-8，避免中文乱码警告
    target_compile_options(EngineCore PRIVATE /MP /utf-8)
    # 禁用某些安全警告（如 fopen 等），因为 Lua 使用传统 C 函数
    target_compile_definitions(EngineCore PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    # GCC / Clang 通用警告选项（严苛但有助于发现潜在问题）
    target_compile_options(EngineCore PRIVATE -Wall -Wextra -pedantic)
    # GNU/Linux 需要的宏定义
    target_compile_definitions(EngineCore PRIVATE _GNU_SOURCE)
endif()

# 设置输出的库文件和可执行文件的目录（统一放在 build/bin 和 build/lib）
set_target_properties(EngineCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"   # 静态库 .lib / .a
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"   # 动态库（本工程未用）
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT_DIR}"       # 👈 可执行文件输出到项目根目录
)

# =============================================================================
# 7. 安装规则（方便部署）
# =============================================================================
# 将引擎的头文件安装到 include/EngineCore/ 下
install(DIRECTORY "${PROJECT_ROOT_DIR}/游戏引擎/"
    DESTINATION "include/EngineCore"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"   # 只安装 .h 和 .hpp
    PATTERN "Private" EXCLUDE                     # 排除 Private 子目录（内部实现）
)

# 将静态库安装到 lib/ 下
install(TARGETS EngineCore
    ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib"
    RUNTIME DESTINATION "bin"
)

# =============================================================================
# 8. 可执行测试目标（用于调试引擎）
# =============================================================================
# 搜索主调文件目录下的 .cpp 文件（通常包含 main() 函数）
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_ROOT_DIR}/游戏引擎/主调文件/*.cpp"
)

if(TEST_SOURCES)
    # 创建测试可执行文件 TestEngine.exe
    add_executable(TestEngine ${TEST_SOURCES})

    # 链接引擎静态库（Lua 和 Sol2 的包含路径通过 PUBLIC 自动继承，无需再写）
    target_link_libraries(TestEngine PRIVATE EngineCore)

    # ---- 新增：将 TestEngine.exe 输出到游戏引擎/ 目录 ----
    set_target_properties(TestEngine PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT_DIR}/游戏引擎"
    )

    # 给 TestEngine 也应用编译选项（保持一致）
    if(MSVC)
        target_compile_options(TestEngine PRIVATE /MP /utf-8)
        target_compile_definitions(TestEngine PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # 将 TestEngine 设为 Visual Studio 的默认启动项目（方便调试）
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "TestEngine")
else()
    # 如果没有主调文件，则只生成静态库，不生成可执行文件（警告但不终止）
    message(WARNING "未找到主调文件目录下的测试源文件，将不会生成可执行文件。")
endif()

# =============================================================================
# 9. 预留插件/模块接口（将来可添加子目录）
# =============================================================================
# add_subdirectory(Plugins/CombatProxy)