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
# 语言：C 和 C++（明确启用 C，以便 .c 文件默认按 C 编译）
project(游戏引擎 LANGUAGES C CXX)

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

    # 3) 排除配置编辑器目录（该目录包含独立的 main()，不应编入静态库）
list(FILTER ENGINE_SOURCES EXCLUDE REGEX "/src/tools/GUI/Config_Editor/")

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

# ---------- 新增：GLFW ----------
set(GLFW_ROOT "${PROJECT_ROOT_DIR}/游戏引擎/external/glfw")
target_include_directories(EngineCore PUBLIC "${GLFW_ROOT}")

# ---------- 新增：GLAD ----------
set(GLAD_ROOT "${PROJECT_ROOT_DIR}/游戏引擎/external/glad")
target_include_directories(EngineCore PUBLIC "${GLAD_ROOT}/include")

# ---------- 新增：Dear ImGui ----------

set(IMGUI_ROOT "${PROJECT_ROOT_DIR}/游戏引擎/external/Dear_ImGui/imgui")
target_include_directories(EngineCore PUBLIC "${IMGUI_ROOT}")

set(IMGUI_BACKENDS "${PROJECT_ROOT_DIR}/游戏引擎/external/Dear_ImGui/backends")
target_include_directories(EngineCore PUBLIC "${IMGUI_BACKENDS}")

# ---------- 新增：公共包含路径打印 ----------

get_target_property(ENGINE_PUBLIC_INCLUDES EngineCore INTERFACE_INCLUDE_DIRECTORIES)
message(STATUS "EngineCore 公共包含路径: ${ENGINE_PUBLIC_INCLUDES}")

# =============================================================================
# 6. 链接第三方库（新增）
# =============================================================================

# 查找 OpenGL 库（跨平台）
find_package(OpenGL REQUIRED)

# 链接 GLFW 静态库和 OpenGL
target_link_libraries(EngineCore PUBLIC
    "${GLFW_ROOT}/glfw3.lib"
    OpenGL::GL
)

# 定义 GLFW_STATIC 宏，确保 GLFW 头文件使用静态库模式
target_compile_definitions(EngineCore PUBLIC GLFW_STATIC)

# 如果是 MSVC，补充 Windows 系统库（GLFW 依赖）
if(MSVC)
    target_link_libraries(EngineCore PUBLIC
        opengl32.lib
        user32.lib
        gdi32.lib
        shell32.lib
    )
endif()

# =============================================================================
# 7. 平台兼容性设置（编译选项、宏定义、输出目录）
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
# 8. 安装规则（方便部署）
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
# 9. 可执行测试目标（用于调试引擎）
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
# 10. 预留插件/模块接口（将来可添加子目录）
# =============================================================================
# add_subdirectory(Plugins/CombatProxy)

# =============================================================================
# 11. 配置编辑器独立可执行文件
# =============================================================================
# 搜索 Config_Editor 目录下的 .cpp 文件（包括 main）
file(GLOB_RECURSE EDITOR_SOURCES
    "${PROJECT_ROOT_DIR}/游戏引擎/src/tools/GUI/Config_Editor/core/*.cpp"
)

if(EDITOR_SOURCES)
    # 创建独立可执行文件
    add_executable(ConfigEditor ${EDITOR_SOURCES})

    # 链接引擎核心静态库（复用所有第三方库和引擎逻辑）
    target_link_libraries(ConfigEditor PRIVATE EngineCore)

    # 应用与 TestEngine 相同的编译选项
    if(MSVC)
        target_compile_options(ConfigEditor PRIVATE /MP /utf-8)
        target_compile_definitions(ConfigEditor PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()

    # 输出到项目根目录（与 TestEngine 并列）
    set_target_properties(ConfigEditor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT_DIR}/游戏引擎"
    )

    message(STATUS "配置编辑器已添加: ConfigEditor.exe")
else()
    message(WARNING "未找到配置编辑器源文件，将不会生成 ConfigEditor.exe。")
endif()