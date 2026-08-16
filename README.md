# 游戏引擎

> 一个基于 **C++20 + Lua** 的轻量级游戏引擎，采用「事件驱动 + 实体组件」架构，内置四叉树空间查询、效应（Buff/技能）系统、数据驱动实体配置与 Lua 行为脚本，并附带一个基于 Dear ImGui 的图形化配置编辑器。

---

## 目录

- [项目简介](#项目简介)
- [核心特性](#核心特性)
- [技术栈](#技术栈)
- [目录结构](#目录结构)
- [核心架构](#核心架构)
- [核心模块详解](#核心模块详解)
- [构建与运行](#构建与运行)
- [配置系统](#配置系统)
- [Lua 脚本系统](#lua-脚本系统)
- [配置编辑器（GUI）](#配置编辑器gui)
- [开发指南](#开发指南)
- [路线图](#路线图)
- [许可证](#许可证)

---

## 项目简介

本项目是一个自研的轻量级游戏引擎，整体设计目标是**数据驱动 + 逻辑可热更新**：

- **实体（Entity）**：引擎中的一切游戏对象（怪物、角色、NPC 等）都是实体，实体分为静态实体与动态实体，动态实体通过 Lua 脚本驱动行为决策。
- **属性槽（Property Slot）**：实体的数值属性（生命、攻击、防御、移速等）统一存放在 `std::unordered_map<std::string, double>` 属性槽中，由 `Property_Manager` 统一管理，C++ 与 Lua 均可读写。
- **事件系统（Event）**：引擎内部模块通过 `Event_Broker`（事件中转器）解耦通信，模块通过 `Event_Terminal`（事件终端）注册订阅与发送事件，事件类型通过「目标模块 + 大类 + 标签」三元组定位。
- **效应系统（Effect）**：技能、Buff、Debuff 等一次性或持续性效果由 `Effect` 描述，`Effect_Manager` 统一调度，支持按时间段（`EffectPhase`）划分执行窗口。
- **空间系统（Space）**：基于四叉树（Quadtree）的 2D 空间索引，由 `Quadtree_Manager` 管理，用于高效的区域查询与碰撞候选筛选。
- **脚本系统（Lua）**：通过 Sol2 将 C++ 类型暴露给 Lua，实体初始化脚本负责写入初始属性，行为决策脚本（决策树）负责每帧的行为逻辑。
- **配置驱动（JSON）**：实体类型、属性槽初始化路径、行为脚本路径等全部由 `assets/config/` 下的 JSON 文件描述，`Config_Loader` 在启动时加载并广播给各模块。

> 当前状态：**开发中（未完成）**。核心模块骨架已可编译运行，实体架构革新正在进行中。

---

## 核心特性

| 特性 | 说明 |
| --- | --- |
| 🧩 **事件驱动架构** | 模块间完全通过事件解耦，支持注册订阅、定向投递、事件仲裁 |
| 🧬 **实体体系** | 抽象基类 `Entity` → 派生 `Dynamic_Entity`，支持从属（minion）权限管理（ACL） |
| 📊 **属性槽机制** | 通用数值属性以 `string → double` 键值对存储，Lua 端可直接读写 |
| ⚡ **效应系统** | 支持效应（Effect）、效应管理器（Effect_Manager）、执行时间段（EffectPhase） |
| 🌳 **四叉树空间索引** | 2D 空间划分与查询，为后续碰撞/索敌提供高效基础 |
| 📜 **Lua 脚本驱动** | Sol2 绑定，初始化脚本 + 行为决策脚本两级脚本体系 |
| 📄 **JSON 数据驱动** | 实体配置、属性配置、行为脚本路径全部外置，无需重编译即可调整 |
| 🖥️ **图形化配置编辑器** | 基于 Dear ImGui + GLFW + OpenGL 的独立工具 `ConfigEditor.exe` |
| 🔧 **现代化 C++20** | ranges、format、numbers、source_location 等 C++20 特性全面启用 |
| 🧪 **双可执行目标** | `TestEngine`（引擎测试入口）与 `ConfigEditor`（配置编辑器）独立构建 |

---

## 技术栈

### 语言与标准

| 项 | 值 |
| --- | --- |
| 语言 | C++20（`CMAKE_CXX_STANDARD 20`，强制要求，禁用扩展） |
| 命名空间 | `engine` |
| 源码风格 | 中文注释 + 中文类名/文件名（如 `实体.h`、`事件中转器.h`） |

### 第三方依赖（`游戏引擎/external/`）

| 库 | 用途 | 引入方式 |
| --- | --- | --- |
| [Lua](https://www.lua.org/) | 脚本语言运行时 | 源码直接编译（`.c` 文件编入引擎） |
| [Sol2](https://github.com/ThePhD/sol2) | C++ ↔ Lua 绑定层 | 纯头文件（`external/Sol2/include`） |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON 解析 | 纯头文件（`external/Json`） |
| [GLFW](https://www.glfw.org/) | 窗口与输入 | 预编译静态库 `glfw3.lib` |
| [GLAD](https://glad.dav1d.de/) | OpenGL 函数加载 | 头文件 + 源码（`external/glad`） |
| [Dear ImGui](https://github.com/ocornut/imgui) | 即时模式 GUI | 源码（`external/Dear_ImGui`） |
| [stb](https://github.com/nothings/stb) | 单头文件图像/字体库 | 单头文件（`external/stb`） |

### 构建工具链

| 项 | 值 |
| --- | --- |
| 构建系统 | CMake ≥ 3.20（使用 `CONFIGURE_DEPENDS` 自动检测源文件变更） |
| 编译器 | MSVC（Visual Studio）、GCC / Clang 跨平台支持 |
| Windows 配置 | Visual Studio「CMake 配置」：`x64-Debug`，生成器 Ninja，继承 `msvc_x64_x64` 环境 |
| 输出目录 | 静态库 → `out/build/<config>/lib`；可执行文件 → `游戏引擎/` 目录 |

---

## 目录结构

> 注意：仓库根目录下存在一层**嵌套的 `游戏引擎/` 子目录**，所有源码与资源都在其中。根目录只放 CMake 工程文件、修复脚本与文档。

```
D:\代码存储\代码仓库\游戏引擎\
├── CMakeLists.txt                # 顶层 CMake 构建脚本（246 行）
├── CMakeSettings.json            # Visual Studio CMake 配置（x64-Debug / Ninja）
├── LICENSE.txt                   # MIT 许可证
├── README.md                     # 本文档
├── fix_a.py                      # 辅助修复脚本
├── fix_config_editor.py          # 配置编辑器修复脚本
├── fix_effect_manager.py         # 效应管理器修复脚本
├── 排除编译代码/                  # 不参与编译的代码存档
├── out/                          # CMake 构建输出（build / install）
│
└── 游戏引擎/                      # ← 引擎本体（所有源码与资源）
    ├── CMakeLists.txt            # （由顶层引用）
    ├── TestEngine.exe            # 引擎测试可执行文件（构建产物）
    ├── ConfigEditor.exe          # 配置编辑器可执行文件（构建产物）
    ├── imgui.ini                 # Dear ImGui 布局配置
    ├── ce_out.log / ce_err.log   # 配置编辑器运行日志
    │
    ├── common/                   # 公共头文件层
    │   ├── 前置头文件包含.h       # 预编译头（标准库 + 第三方库统一引入）
    │   ├── 引擎总头文件.h         # 引擎聚合头（一键包含全部核心模块）
    │   ├── external/             # 公共外部库封装
    │   │   └── Sol2/             #   Sol2 类型注册 / 类型别名
    │   └── types/                # 全局类型定义
    │       ├── 事件类型.h         #   event / config_event + 哈希特化
    │       ├── 坐标类型.h         #   坐标相关类型
    │       └── 计时器类型.h       #   计时器相关类型
    │
    ├── src/                      # 引擎源码
    │   ├── core/                 # ★ 核心模块
    │   │   ├── entity/           #   实体系统
    │   │   │   ├── Entity/               #     实体基类（抽象）
    │   │   │   ├── Dynamic_Entity/       #     动态实体（Lua 决策树驱动）
    │   │   │   ├── Entity_Manager/       #     实体管理器（创建/卸载/行动）
    │   │   │   └── Property_Manager/     #     属性槽管理器
    │   │   ├── event/            #   事件系统
    │   │   │   ├── Event_Broker/         #     事件中转器（模块解耦中枢）
    │   │   │   └── Event_Terminal/       #     事件终端（订阅/发送）
    │   │   ├── effect/           #   效应系统
    │   │   │   ├── Effect/               #     效应基类
    │   │   │   ├── Effect_Manager/       #     效应管理器
    │   │   │   └── EffectPhase/          #     效应执行时间段
    │   │   └── space/            #   空间系统
    │   │       ├── Quadtree/             #     四叉树（空间划分）
    │   │       └── Quadtree_Manager/     #     四叉树管理器
    │   │
    │   └── tools/                # 工具模块
    │       ├── GUI/              #   图形界面工具
    │       │   └── Config_Editor/        #     配置编辑器（独立 exe）
    │       │       ├── 配置编辑器.h
    │       │       ├── 实体配置模型.h
    │       │       └── core/             #     编辑器核心实现
    │       └── Non_GUI/          #   非图形工具
    │           ├── Auxi_Algorithm/       #     算法辅助（binary_search 等）
    │           ├── Config_Checker/       #     配置检查器
    │           ├── Config_Loader/        #     配置加载器
    │           ├── Engine_Env/           #     引擎环境
    │           ├── Input_Processer/      #     输入处理器
    │           ├── Logging/              #     日志系统
    │           ├── Random/               #     随机数生成器
    │           └── Timer/                #     计时器
    │
    ├── 主调文件/                 # 引擎测试入口（main）
    │   └── 主调文件.cpp
    │
    ├── assets/                   # 资源与配置（数据驱动核心）
    │   ├── config/               #   JSON 配置
    │   │   ├── entities/         #     实体类型配置（7 个实体）
    │   │   ├── property/         #     属性槽初始化配置
    │   │   ├── format/           #     格式定义
    │   │   └── route/            #     路径/路由配置
    │   ├── scripts/              #   Lua 脚本
    │   │   ├── initialize/       #     实体初始化脚本
    │   │   └── behavior/         #     行为决策脚本
    │   └── UI/                   #   UI 资源
    │
    └── external/                 # 第三方库（源码级引入）
        ├── Dear_ImGui/           #   Dear ImGui（imgui + backends）
        ├── glad/                 #   OpenGL 加载器
        ├── glfw/                 #   GLFW（含预编译 glfw3.lib）
        ├── Json/                 #   nlohmann/json
        ├── Lua/                  #   Lua 官方源码
        ├── Sol2/                 #   Sol2 头文件
        └── stb/                  #   stb 单头文件库
```

---

## 核心架构

引擎采用「**分层 + 事件总线**」的结构，模块之间不直接互相持有引用，而是通过 `Event_Broker` 中转事件：

```
                     ┌─────────────────────────────────────┐
                     │           Event_Broker              │
                     │         （事件中转器）                │
                     │    info_register / receive          │
                     └──────┬──────────┬──────────┬────────┘
                            │          │          │
                     ┌──────▼───┐ ┌────▼─────┐ ┌──▼──────────┐
                     │Config_   │ │Property_ │ │Entity_      │
                     │Loader    │ │Manager   │ │Manager      │
                     └──────────┘ └──────────┘ └──┬──────────┘
                                                   │ 创建/管理
                                            ┌──────▼──────┐
                                            │Dynamic_     │
                                            │Entity       │
                                            │(Lua决策树)   │
                                            └──────┬──────┘
                                                   │ 读写
                                            ┌──────▼──────┐
                                            │Property_Slot│
                                            │(属性槽)      │
                                            └─────────────┘
```

### 启动流程（`主调文件/主调文件.cpp`）

1. 控制台切换 UTF-8 编码（`SetConsoleOutputCP(CP_UTF8)` / `SetConsoleCP(CP_UTF8)`）；
2. 构造五个核心对象：`Event_Broker`、`Config_Loader`、`Property_Manager`、`Effect_Manager`、`Entity_Manager`；
3. 通过两个 lambda 封装事件中转站的**接入入口**（`attach_entry`）与**事件入口**（`event_entry`）：
   - `attach_entry(name, events, event_entry)` → 转发给 `event_broker.info_register`，供模块注册订阅；
   - `event_entry(event_set)` → 转发给 `event_broker.receive`，供模块批量投递事件；
4. **属性槽管理器**：注入接入入口 → `attach()` 接入事件中转站 → 封装属性槽获取通道 `prop_bind_entry(ID)`；
5. **效应管理器**：注入接入入口 + 事件入口 → `attach()` → 注入属性槽绑定通道；
6. **实体管理器**：注入接入入口 + 事件入口 → `attach()` → 注入属性槽绑定通道；
7. **配置加载器**：注入事件入口 → `act()` 加载 `assets/config/` 下全部配置并广播；
8. 进入 `for (;;)` 主循环（后续将接入帧循环与实体行动驱动）。

### 依赖注入模式

引擎模块之间通过 **std::function 回调注入**（依赖注入）解耦：

| 通道 | 类型 | 用途 |
| --- | --- | --- |
| `attach_entry` | `(name, events, entry) → void` | 模块向事件中转站注册订阅 |
| `event_entry` | `(event_set) → void` | 模块向事件中转站投递事件 |
| `prop_bind_entry` | `(ID) → unordered_map<string,double>*` | 实体获取自己的属性槽 |

---

## 核心模块详解

### 1. 实体系统（`src/core/entity/`）

#### 实体基类 `Entity`（`Entity/实体.h`）

抽象基类，**不可直接创建**，提供所有实体的公共身份信息：

| 成员 | 类型 | 说明 |
| --- | --- | --- |
| `type` | `std::string` | 实体类型标签（如 `"Goblin"`） |
| `ID` | `int64_t` | 实体编号（唯一标识） |
| `alive` | `bool` | 存活标记 |
| `ID_get()` | `int64_t` | 获取实体 ID |
| `type_get()` | `std::string` | 获取实体类型 |
| `is_alive()` | `bool` | 查询存活状态 |

#### 动态实体 `Dynamic_Entity`（`Dynamic_Entity/动态实体.h`）

继承 `Entity`，是可被 Lua 脚本驱动的活动实体：

| 成员 | 类型 | 说明 |
| --- | --- | --- |
| `property_slot` | `unordered_map<string,double>*` | 通用属性槽指针（由 Property_Manager 分配） |
| `minion_set` | `unique_ptr<vector<minion_record>>` | 从属实体记录集合 |
| `event_terminal` | `Event_Terminal` | 该实体的事件终端 |
| `acl_key` | `int64_t` | 权限密钥（ACL 校验） |
| `decision_tree` | `LuaState` | Lua 决策树状态（行为脚本运行时） |

关键接口：

- `prop_slot_bind(ptr)`：绑定属性槽；
- `decision_tree_load(path)`：加载行为决策 Lua 脚本；
- `act()`：**行为决策**——每帧由实体管理器调用，驱动 Lua 决策树执行；
- `event_govern(event)`：事件仲裁（私有，处理定向投递给该实体的事件）。

#### 实体管理器 `Entity_Manager`（`Entity_Manager/实体管理器.h`）

引擎中所有实体的「户籍管理处」：

| 数据 | 说明 |
| --- | --- |
| `entity_set` | 活跃实体集合（`entity_record{ID, shared_ptr<Dynamic_Entity>}`） |
| `acl_set` | 从属权限集合（`ownership_acl{master, minion_set}`） |
| `minion_records` | 从属关系记录（`minion_record{master, minion_set}`） |
| `event_map` | 订阅事件集合（`unordered_set<config_event>`） |
| `decision_load_paths` | 实体类型 → 决策树加载路径映射 |
| `start_ID / now_ID` | 实体 ID 分配器（从 10000 起） |

关键接口：

- `entity_build(type, counts)` / `entity_build(type, IDs)`：批量创建实体；
- `entity_unload(IDs)`：卸载实体；
- `entity_act()`：驱动所有活跃实体执行行为决策；
- `outer_event_process(evt)`：处理外部事件；
- `config_field_parse(config)`：解析并校验实体配置文件。

#### 属性槽管理器 `Property_Manager`（`Property_Manager/属性槽管理器.h`）

管理所有实体的通用属性槽（`unordered_map<string,double>`），提供：

- `prop_slot_get(ID)`：按实体 ID 获取属性槽指针；
- 事件接入：响应配置事件，为实体建立属性槽并触发初始化脚本。

### 2. 事件系统（`src/core/event/`）

#### 事件类型（`common/types/事件类型.h`）

```cpp
namespace engine {
    // 抽象事件（不可直接创建）
    struct event {
        std::string target_module;  // 目标接收模块
        std::string category;       // 事件大类
        std::string tag;            // 类内标签
        virtual ~event() = 0;
    };

    // 配置事件（携带 JSON 配置包）
    struct config_event : public event {
        nlohmann::json config;      // 配置数据
    };
}
```

- `std::hash<engine::event>` / `std::hash<engine::config_event>` 已特化（含 `detail::hash_combine` 组合哈希），事件可作为 `unordered_set` / `unordered_map` 键使用；
- 事件以「目标模块 + 大类 + 标签」三元组唯一定位，实现模块间定向通信。

#### 事件终端 `Event_Terminal`（`Event_Terminal/事件终端.h`）

每个模块/实体持有的事件收发接口：

- `attach_entry_register(entry)`：注册接入入口（连接中转站）；
- `event_entry_register(entry)`：注册事件入口（投递通道）；
- 负责本模块的订阅注册与事件投递。

#### 事件中转器 `Event_Broker`（`Event_Broker/事件中转器.h`）

全局事件中枢：

- `info_register(name, events, entry)`：模块注册订阅（名称 + 关注的事件集 + 回调）；
- `receive(event_set)`：接收事件集合并分发给匹配的订阅者。

### 3. 效应系统（`src/core/effect/`）

| 组件 | 文件 | 职责 |
| --- | --- | --- |
| `Effect` | `Effect/效应.h` | 效应基类：描述技能/Buff/Debuff 的效果数据与逻辑 |
| `Effect_Manager` | `Effect_Manager/效应管理器.h` | 效应调度中心：管理效应生命周期，绑定属性槽通道，响应配置事件 |
| `EffectPhase` | `EffectPhase/效应执行时间段.h` | 效应执行的时间段划分（如施法前摇 / 持续期 / 结算期） |

### 4. 空间系统（`src/core/space/`）

| 组件 | 文件 | 职责 |
| --- | --- | --- |
| `Quadtree` | `Quadtree/四叉树.h` | 四叉树节点结构与插入/查询算法；`四叉树通信结构体.h` 定义查询输入输出；`函数预声明.h` 声明算法接口 |
| `Quadtree_Manager` | `Quadtree_Manager/四叉树管理器.h` | 四叉树管理入口：整体划分、区域查询、对象管理；`四叉树管理器通信结构体.h` 定义管理通信协议 |

### 5. 非图形工具（`src/tools/Non_GUI/`）

| 模块 | 职责 |
| --- | --- |
| `Auxi_Algorithm` | 算法辅助工具（如 binary_search 等通用算法） |
| `Config_Checker` | 配置检查器：校验 JSON 配置字段合法性 |
| `Config_Loader` | 配置加载器：启动时加载 `assets/config/` 全部配置并广播事件 |
| `Engine_Env` | 引擎环境：路径、环境变量等运行环境信息 |
| `Input_Processer` | 输入处理器：键盘/鼠标输入收集与分发 |
| `Logging` | 日志系统：引擎运行日志 |
| `Random` | 随机数生成器 |
| `Timer` | 计时器：帧时间、倒计时等 |

---

## 构建与运行

### 环境要求

- **CMake** ≥ 3.20（顶层脚本使用了 `CONFIGURE_DEPENDS` 等特性）
- **C++20 编译器**：MSVC（Visual Studio 2022）或 GCC / Clang
- **OpenGL**（GLFW / GLAD 依赖）
- Windows 下需安装「使用 C++ 的桌面开发」工作负载（含 MSVC 工具集）

### 构建步骤

#### 方式一：Visual Studio（推荐，Windows）

1. 用 VS 打开仓库根目录（含 `CMakeLists.txt` 的目录）；
2. VS 会读取 `CMakeSettings.json`，自动生成 **x64-Debug（Ninja）** 配置；
3. 选择「生成全部」，产出：

| 目标 | 产物 | 位置 |
| --- | --- | --- |
| `EngineCore`（静态库） | `EngineCore.lib` | `out/build/x64-Debug/lib/` |
| `TestEngine`（可执行） | `TestEngine.exe` | `游戏引擎/` |
| `ConfigEditor`（可执行） | `ConfigEditor.exe` | `游戏引擎/` |

#### 方式二：命令行

```bash
cmake -S . -B out/build/x64-Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug
```

#### 方式三：跨平台（GCC / Clang）

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

> 注：跨平台构建时需自行准备 GLFW 静态库（当前 `glfw3.lib` 为 Windows 预编译产物）。

### 运行

- **TestEngine**：引擎功能测试入口，启动后加载配置并进入主循环（当前为 `for(;;)` 空循环，等待帧循环接入）；
- **ConfigEditor**：独立配置编辑器（Dear ImGui 界面），用于可视化编辑实体 JSON 配置。

### 顶层 CMake 目标解析（`CMakeLists.txt`）

| 区块 | 内容 |
| --- | --- |
| 源文件收集 | `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` 收集 `游戏引擎/**/*.cpp|.c`，自动排除构建目录、`主调文件/`、`src/tools/GUI/Config_Editor/` |
| `EngineCore` | 静态库，PUBLIC 传播所有第三方 include 路径 |
| 第三方库 | Sol2 / Lua（源码编译）/ Json / GLFW / GLAD / Dear ImGui / stb |
| 链接 | `glfw3.lib` + `OpenGL::GL` + MSVC 系统库（opengl32 / user32 / gdi32 / shell32） |
| 编译选项 | MSVC：`/MP /utf-8`、`_CRT_SECURE_NO_WARNINGS`；GCC/Clang：`-Wall -Wextra -pedantic`、`_GNU_SOURCE` |
| 输出目录 | 库 → `out/build/<cfg>/lib`；可执行 → `游戏引擎/` |
| 安装规则 | 头文件（排除 Private）→ `include/EngineCore/`；库 → `lib/` |
| `TestEngine` | 编译 `主调文件/*.cpp`，链接 `EngineCore`，设为 VS 默认启动项目 |
| `ConfigEditor` | 编译 `src/tools/GUI/Config_Editor/core/*.cpp`，链接 `EngineCore` |
| 预留接口 | 第 10 节注释保留 `add_subdirectory(Plugins/CombatProxy)` 插件扩展位 |

---

## 配置系统

### 配置目录（`assets/config/`）

| 目录 | 内容 |
| --- | --- |
| `entities/` | 实体类型定义（type / acls / needed_events） |
| `property/` | 属性槽初始化路径（type / initialize_path） |
| `format/` | 格式定义 |
| `route/` | 路径 / 路由配置 |

### 实体配置示例（`entities/哥布林 (Goblin).json`）

```json
{
  "type": "Goblin",
  "acls": {
    "master": "Goblin",
    "minion_set": []
  },
  "needed_events": [
    { "category": "Entity", "tag": "Request" },
    { "category": "config", "tag": "damage" }
  ]
}
```

| 字段 | 说明 |
| --- | --- |
| `type` | 实体类型标识（对应 Lua 初始化脚本命名） |
| `acls.master` | 从属权限：主实体类型 |
| `acls.minion_set` | 允许的从属实体类型集合 |
| `needed_events` | 该实体需要订阅的事件（category + tag） |

### 属性配置示例（`property/哥布林 (Goblin).json`）

```json
{
  "type": "Goblin",
  "initialize_path": "scripts/initialize/哥布林 (Goblin).lua"
}
```

配置加载器启动时读取所有 JSON → 构造 `config_event` → 通过事件中转站广播 → 各管理器按订阅响应。

---

## Lua 脚本系统

### 绑定层（`common/external/Sol2/`）

- `sol类型别名.h`：预定义 Sol2 库类型别名（如 `LuaState`）；
- `sol类型注册.h`：C++ 类型注册到 Lua 的方法封装。

### 两级脚本体系（`assets/scripts/`）

#### ① 初始化脚本（`initialize/`）

实体创建时由 `Property_Manager` 调用，向通用属性槽写入初始数值，并登记行为脚本路径。

```lua
-- scripts/initialize/哥布林 (Goblin).lua
function Goblin_Initialize(entity)
    local pros = entity.pros          -- 通用属性槽引用

    pros.max_hp = 100.0               -- 最大生命值
    pros.now_hp = 100.0               -- 当前生命值
    pros.attack_power = 12.0          -- 攻击力
    pros.defense = 3.0                -- 防御力
    pros.move_speed = 4.5             -- 移动速度
    pros.attack_range = 1.5           -- 攻击范围
    pros.attack_cooldown = 1.2        -- 攻击间隔（秒）
    pros.sight_range = 10.0           -- 索敌视野
    pros.state_giddy = 0.0            -- 眩晕状态计时
    pros.state_frozen = 0.0           -- 冰冻状态计时

    entity.behavior_script_path =
        "scripts/behavior/哥布林 (Goblin)_Behavior.lua"   -- 行为脚本登记
end
```

#### ② 行为决策脚本（`behavior/`）

由 `Dynamic_Entity::decision_tree_load()` 加载，作为实体的「决策树」，在 `act()` 每帧驱动下执行索敌、攻击、移动等行为逻辑。

### 属性槽在 Lua 中的读写约定

- 属性名使用**小写下划线**命名（如 `max_hp`、`attack_power`）；
- 所有属性值为 `double`；
- 状态类属性约定为 `state_*` 前缀（`state_giddy`、`state_frozen`），数值表示剩余持续时间。

---

## 配置编辑器（GUI）

`ConfigEditor.exe` 是基于 **Dear ImGui + GLFW + OpenGL** 的独立图形化工具，用于可视化编辑实体配置：

| 文件 | 说明 |
| --- | --- |
| `配置编辑器.h` | 编辑器主界面与交互逻辑（9.3KB） |
| `实体配置模型.h` | 实体配置的数据模型（18.2KB） |
| `core/` | 编辑器核心实现（由顶层 CMake 单独编译为独立 exe） |

运行日志输出到 `游戏引擎/ce_out.log` / `ce_err.log`，ImGui 布局保存于 `imgui.ini`。

---

## 开发指南

### 如何新增一个实体类型

1. **编写实体配置**：在 `assets/config/entities/` 新建 `<类型名>.json`，声明 `type` / `acls` / `needed_events`；
2. **编写属性配置**：在 `assets/config/property/` 新建同名 JSON，指向初始化脚本路径；
3. **编写初始化脚本**：在 `assets/scripts/initialize/` 新建 `<类型名>.lua`，函数名约定为 `<类型名>_Initialize(entity)`，写入初始属性并登记行为脚本路径；
4. **编写行为脚本**（可选）：在 `assets/scripts/behavior/` 新建行为决策脚本，由初始化脚本登记路径；
5. 重新运行 `TestEngine`，`Config_Loader` 会自动加载新配置（无需重编译）。

### 如何新增一个引擎模块

1. 在 `src/core/` 或 `src/tools/Non_GUI/` 下创建模块目录（含 `局部命名空间使用.h`、核心头文件、`core/` 实现目录）；
2. 若需要与其他模块通信：持有 `Event_Terminal event_terminal` 成员；
3. 在 `common/引擎总头文件.h` 中登记该模块的头文件；
4. 在 `主调文件.cpp` 中按「注入 attach_entry → 注入 event_entry → attach() → 注入依赖通道」的顺序初始化；
5. 源码文件会被 CMake 的 `CONFIGURE_DEPENDS` 自动收集，直接构建即可。

### 代码风格约定

- 命名空间统一为 `engine`；
- 文件名 / 类名 / 注释使用中文（如 `实体.h`、`Effect_Manager` 目录）；
- 目录级命名约定：`模块名_Manager`（管理器）、`模块名_Broker`（中转器）、`模块名_Terminal`（终端）；
- 每个模块目录通常包含：`局部命名空间使用.h`（模块内 using 声明）、`core/`（实现细节）；
- 事件 category / tag 使用英文大写驼峰（如 `Entity` / `Request`）。

---

## 路线图

- [ ] **实体架构革新（进行中）**：重构实体体系，完善 `Entity` / `Dynamic_Entity` / `Entity_Manager` 关系
- [ ] 主循环接入：将 `for(;;)` 空循环替换为帧循环（固定时间步 + 更新驱动）
- [ ] 渲染管线：接入 GLFW / GLAD / OpenGL 渲染循环
- [ ] 四叉树空间查询接入实体系统（索敌 / 碰撞候选）
- [ ] 效应系统完整生命周期（EffectPhase 驱动的施放 / 持续 / 结算）
- [ ] 配置编辑器增强：实体可视化编辑、属性预览、脚本关联
- [ ] Lua 行为决策树完善（状态机 / 行为树）
- [ ] 插件化模块接口（`add_subdirectory(Plugins/...)`）

---

## 许可证

[MIT License](LICENSE.txt)

```
MIT License

Copyright (c) [year] [fullname]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
```

---

*本文档由对仓库实际源码、构建脚本与配置的完整扫描生成，所有目录与接口均经逐一核实。*
