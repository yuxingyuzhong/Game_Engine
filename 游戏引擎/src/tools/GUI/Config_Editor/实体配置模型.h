#pragma once
//============================================================================
// 实体配置模型 —— 配置编辑器数据层
// ---------------------------------------------------------------------------
// 职责：
//   1. 定义实体配置的数据模型（与 Entity_Manager::config_field_parse
//      期望的 JSON 格式一一对应）
//   2. 提供仓库类：从磁盘加载 / 保存实体配置，并同步路由表
//      （assets/config/route/entity.json）
//   3. 提供与引擎 Config_Checker::field_check 一致的校验逻辑
//
// 引擎期望的标准 JSON 格式：
//   实体配置（Entity_Manager::config_field_parse）：
//   {
//     "type": "Goblin",                             // string，非空
//     "decision_load_path": "scripts/xxx.lua",      // string，非空（决策树行为脚本，Entity_Manager 用）
//     "acls": ["Goblin_Priest"],                    // vector<string>，非空
//     "needed_events": [["Entity","Request"]]       // vector<pair<string,string>>，非空
//   }
//   属性槽配置（Property_Manager::event_process）：
//   {
//     "type": "Goblin",                             // string，非空
//     "initialize_path": "scripts/xxx.lua"          // string，非空（属性槽初始化，Property_Manager 用）
//   }
// 注意：Entity_Manager 的 config_field_parse 现在要求 decision_load_path
//       （旧格式仅 initialize_path，引擎已无法加载旧配置，编辑器兼容读取并在校验中提示）。
//============================================================================

#include "common/前置头文件包含.h"

//引擎命名空间
namespace engine
{
    //========================================================================
    // 实体配置数据模型
    //========================================================================
    struct 实体配置
    {
        //实体类型标识（对应 JSON 字段 "type"）
        std::string type;
        //决策树加载路径（对应 JSON 字段 "decision_load_path"，
        //  由 Entity_Manager 注册决策树，Dynamic_Entity 加载行为脚本）
        std::string decision_load_path;
        //实体从属权限列表（对应 JSON 字段 "acls"，作为 Entity_Manager
        //  owner_acl_register 的 minion_set，master 即实体类型自身）
        std::vector<std::string> acls;
        //实体订阅事件列表（对应 JSON 字段 "needed_events"，
        //  每个元素为 {分类, 标签}，如 {"Entity","Request"}）
        std::vector<std::pair<std::string, std::string>> needed_events;

        //配置文件的相对路径（相对 assets/ 目录，如 "config/entities/史莱姆 (Slime).json"）
        //该字段仅编辑器内部使用，不写入 JSON
        std::string config_path;
    };

    //========================================================================
    // 属性槽配置数据模型（Property_Manager 专属配置文件）
    // -----------------------------------------------------------------------
    // 属性槽初始化路径不再与实体配置同存一份，而是独立成第二种配置文件：
    //   config/property/<type>.json
    // 引擎 Property_Manager::event_process 只读取 type 与 initialize_path
    // 两个字段；实体配置的解析由 Entity_Manager 负责，两者彻底分离。
    //========================================================================
    struct 属性槽配置
    {
        //实体类型标识（对应 JSON 字段 "type"，与实体配置的 type 一致）
        std::string type;
        //属性槽初始化脚本路径（对应 JSON 字段 "initialize_path"，
        //  由 Property_Manager 用于构建属性槽，调用 initialize()）
        std::string initialize_path;

        //配置文件的相对路径（相对 assets/ 目录，如 "config/property/史莱姆 (Slime).json"）
        //该字段仅编辑器内部使用，不写入 JSON
        std::string config_path;
    };

    //========================================================================
    // 配置格式系统 —— 目标模块 ↔ 配置文件格式
    // -----------------------------------------------------------------------
    // 创建配置文件时先指定「目标模块」，编辑器按该模块的格式定义自动生成
    // 并校验配置。内置两个模块格式（Entity_Manager / Property_Manager），
    // 用户可通过「配置格式管理」窗口创建新目标模块并自定义字段格式。
    // 格式定义持久化到 assets/config/format/<模块名>.json，重启后依然可用。
    //========================================================================

    //字段类型（决定编辑控件与 JSON 序列化形式）
    enum class 配置字段类型
    {
        文本,          //string：普通文本输入框
        脚本路径,      //script：文本 + assets/scripts 下拉选择器
        文本列表,      //string_list：字符串数组编辑列表
        事件对列表,    //pair_list：二元组数组（分类,标签），如 needed_events
        整数,          //int：整数值
        浮点数,        //float：浮点数值
        布尔,          //bool：勾选框
    };

    //单个字段的定义（配置格式的组成单元）
    struct 配置字段定义
    {
        //JSON 键名（如 "type"、"decision_load_path"）
        std::string 字段名;
        //界面显示名（可空，空时直接用字段名）
        std::string 显示名;
        //字段类型
        配置字段类型 类型 = 配置字段类型::文本;
        //是否必填（必填字段缺失/为空时校验报错）
        bool 必填 = true;
        //字段说明（界面悬停/说明区展示）
        std::string 说明;
    };

    //配置格式（一个目标模块对应一种格式）
    struct 配置格式
    {
        //目标模块名（如 "Entity_Manager"、"Property_Manager" 或自定义模块名）
        std::string 模块名;
        //配置文件相对 config/ 的子目录（如 "entities"、"property"、"custom/MyModule"）
        std::string 配置目录;
        //路由文件名（route/ 下，如 "entity.json"、"custom_MyModule.json"）
        std::string 路由文件名;
        //内置格式标记（内置格式仅可查看，不可编辑/删除）
        bool 内置 = false;
        //字段定义列表（顺序即界面编辑顺序）
        std::vector<配置字段定义> 字段;
    };

    //通用配置条目（自定义模块的配置文件）
    struct 通用配置
    {
        //所属模块名（对应某个 配置格式::模块名）
        std::string 模块名;
        //条目名（UI 显示 / 文件名基，如实体格式的 type 值）
        std::string 条目名;
        //整个配置的字段值（JSON 对象：{"type":"Goblin", ...}）
        nlohmann::json 字段值;
        //配置文件的相对路径（相对 assets/ 目录）
        std::string config_path;
    };

    //========================================================================
    // 实体配置仓库
    //========================================================================
    class 实体配置仓库
    {
    private:
        //资源基目录（exe 同级 assets/，与 Config_Loader::base_dir_get 一致）
        std::filesystem::path assets_dir;
        //路由文件名（固定管理该文件）
        std::filesystem::path route_path;
        //实体配置集合（保持路由顺序）
        std::vector<实体配置> 实体集合;
        //路由表原始 JSON（用于保存时最小化改动）
        nlohmann::json route_json;
        //属性槽路由文件名（固定管理该文件）
        std::filesystem::path property_route_path;
        //属性槽配置集合（保持路由顺序）
        std::vector<属性槽配置> 属性槽集合;
        //属性槽路由表原始 JSON（用于保存时最小化改动）
        nlohmann::json property_route_json;
        //配置格式定义目录（assets/config/format/）
        std::filesystem::path format_dir;
        //配置格式集合（内置 + 自定义，顺序即列表顺序）
        std::vector<配置格式> 格式集合;
        //通用配置集合（自定义模块的配置条目）
        std::vector<通用配置> 通用配置集合;
        //自定义模块路由表原始 JSON 缓存（模块名 → 路由数组）
        std::map<std::string, nlohmann::json> 自定义路由表;
        //最近一次加载时跳过的损坏/缺失配置文件数量（状态栏提示用）
        int 上次加载跳过数 = 0;

    public:
        //构造函数：定位资源目录
        实体配置仓库();

        // ———— 加载 ————

        //从磁盘加载全部实体配置（读取路由表 + 各实体 JSON）
        //返回是否成功（失败时错误信息写入 status_message）
        bool 加载();

        // ———— 查询 ————

        //获取全部实体配置（只读）
        const std::vector<实体配置>& 获取全部() const;
        //获取全部实体配置（可修改，编辑器保存/编辑用）
        std::vector<实体配置>& 获取全部();
        //按类型查找实体配置（未找到返回 nullptr）
        实体配置* 查找类型(const std::string& type);
        //获取脚本相对路径列表（扫描 assets/scripts 下所有 .lua）
        std::vector<std::string> 获取脚本列表() const;
        //获取资源目录
        const std::filesystem::path& 资源目录() const;
        //最近一次加载时跳过的损坏/缺失配置文件数量（路由条目损坏 / 文件缺失 / JSON 解析失败等）
        int 获取上次跳过数() const { return 上次加载跳过数; }

        // ———— 属性槽配置（config/property/，与实体配置分离） ————

        //获取全部属性槽配置（只读）
        const std::vector<属性槽配置>& 获取属性槽全部() const;
        //获取全部属性槽配置（可修改，编辑器保存/编辑用）
        std::vector<属性槽配置>& 获取属性槽全部();
        //按类型查找属性槽配置（未找到返回 nullptr）
        属性槽配置* 查找属性槽(const std::string& type);
        //校验属性槽配置（type 非空、initialize_path 非空 + 脚本存在性软检查）
        bool 校验属性槽(const 属性槽配置& cfg,
            std::vector<std::string>& errors,
            std::vector<std::string>& warnings) const;
        //保存单个属性槽配置：写入属性槽 JSON + 同步属性槽路由表
        bool 保存属性槽(属性槽配置& cfg, std::string& error);
        //新建属性槽配置：写入新属性槽 JSON + 追加路由条目
        bool 新建属性槽(属性槽配置& cfg, std::string& error);
        //删除属性槽配置：移除路由条目（属性槽文件保留，避免误删）
        bool 删除属性槽(const std::string& type, std::string& error);

        // ———— 配置格式（目标模块 ↔ 格式定义） ————

        //获取全部配置格式（只读）
        const std::vector<配置格式>& 获取格式全部() const;
        //获取全部配置格式（可修改，格式管理窗口编辑用）
        std::vector<配置格式>& 获取格式全部();
        //按模块名查找配置格式（未找到返回 nullptr）
        配置格式* 查找格式(const std::string& module);
        //加载全部格式定义：首次运行时自动生成内置格式种子（Entity_Manager / Property_Manager），
        //随后读取 assets/config/format/ 下全部格式文件
        bool 加载格式();
        //保存格式定义：新建（自定义模块）或覆盖已有（内置格式拒绝覆盖）
        //返回是否成功，错误信息写入 error
        bool 保存格式(const 配置格式& fmt, std::string& error);
        //删除格式定义：仅允许自定义格式（内置格式拒绝删除）
        //返回是否成功，错误信息写入 error
        bool 删除格式(const std::string& module, std::string& error);
        //扫描并删除未被任何路由表引用的孤儿配置文件（config/entities、config/property、config/custom 下）
        //返回删除的文件数量，被删文件相对路径写入 删除列表；失败返回 -1 并写入 error
        int 清理孤儿配置(std::vector<std::string>& 删除列表, std::string& error);
        //字段类型 → 显示名（如 文本/脚本路径/文本列表/事件对列表/整数/浮点数/布尔）
        static const char* 字段类型名称(配置字段类型 type);
        //字段类型 → JSON 序列化键名（string/script/string_list/pair_list/int/float/bool）
        static const char* 字段类型键(配置字段类型 type);
        //按 JSON 键名解析字段类型（未知键名回退为 文本）
        static 配置字段类型 解析字段类型(const std::string& key);

        // ———— 通用配置（自定义模块） ————

        //获取全部通用配置（只读）
        const std::vector<通用配置>& 获取通用配置全部() const;
        //获取全部通用配置（可修改）
        std::vector<通用配置>& 获取通用配置全部();
        //按模块名 + 条目名查找通用配置（未找到返回 nullptr）
        通用配置* 查找通用配置(const std::string& module, const std::string& name);
        //新建通用配置：按格式默认字段值写入 JSON + 追加自定义模块路由
        bool 新建通用配置(通用配置& cfg, std::string& error);
        //保存通用配置：写入 JSON + 同步自定义模块路由（条目名变更时自动迁移文件）
        bool 保存通用配置(通用配置& cfg, std::string& error);
        //删除通用配置：移除自定义模块路由条目（文件保留）
        bool 删除通用配置(const std::string& module, const std::string& name, std::string& error);
        //校验通用配置（按格式字段定义：必填/类型/脚本存在性软检查）
        bool 校验通用配置(const 配置格式& fmt, const 通用配置& cfg,
            std::vector<std::string>& errors, std::vector<std::string>& warnings) const;
        //按格式定义生成默认字段值（JSON 对象，缺失字段补默认值）
        static nlohmann::json 生成默认字段值(const 配置格式& fmt);
        //从字段值中提取条目名（优先取 type/name 字段，否则用空串由调用方生成）
        static std::string 提取条目名(const nlohmann::json& data);

        // ———— 校验 ————

        //校验配置（复刻 Config_Checker::field_check 的非空/类型检查逻辑）
        //错误列表写入 errors，警告列表写入 warnings；返回是否有致命错误
        bool 校验(const 实体配置& cfg,
            std::vector<std::string>& errors,
            std::vector<std::string>& warnings) const;

        // ———— 保存 ————

        //保存单个实体配置：写入实体 JSON + 同步路由表
        //type 变更时自动迁移文件（旧文件重命名为新路径）
        //返回是否成功，错误信息写入 error
        bool 保存实体(实体配置& cfg, std::string& error);
        //新建实体配置：写入新实体 JSON + 追加路由条目
        //返回是否成功，错误信息写入 error
        bool 新建实体(实体配置& cfg, std::string& error);
        //删除实体配置：移除路由条目（实体文件保留，避免误删）
        //返回是否成功，错误信息写入 error
        bool 删除实体(const std::string& type, std::string& error);

    private:
        //读取路由表（route/entity.json）
        bool 读取路由();
        //写入路由表（route/entity.json）
        bool 写入路由();
        //解析单个实体 JSON（兼容旧格式：acls 可为对象/数组，needed_events 可为对象数组）
        bool 解析实体配置(const nlohmann::json& data, 实体配置& out, std::string& error);
        //序列化实体配置为标准 JSON（引擎期望格式）
        nlohmann::json 序列化实体配置(const 实体配置& cfg) const;
        //按类型生成默认配置路径（config/entities/<type>.json）
        std::string 生成配置路径(const std::string& type) const;
        //字符串清理（剔除路径非法字符）
        static std::string 文件名清理(const std::string& name);

        //读取属性槽路由表（route/property.json）
        bool 读取属性槽路由();
        //写入属性槽路由表（route/property.json）
        bool 写入属性槽路由();
        //解析单个属性槽 JSON
        bool 解析属性槽配置(const nlohmann::json& data, 属性槽配置& out, std::string& error);
        //序列化属性槽配置为标准 JSON
        nlohmann::json 序列化属性槽配置(const 属性槽配置& cfg) const;
        //按类型生成属性槽默认配置路径（config/property/<type>.json）
        std::string 生成属性槽配置路径(const std::string& type) const;

        //读取单个格式定义文件（path 为 format/ 下文件，解析失败返回 false）
        bool 读取格式文件(const std::filesystem::path& path, 配置格式& out);
        //写入单个格式定义文件（format/<模块名>.json）
        bool 写入格式文件(const 配置格式& fmt);
        //确保内置格式种子存在（Entity_Manager / Property_Manager，缺失时自动生成）
        void 确保内置格式();
        //加载单个自定义模块的全部通用配置（读取路由 + 各配置 JSON）
        //返回本次跳过的损坏/缺失配置数量（路由条目损坏 / 文件缺失 / JSON 解析失败等）
        int 加载模块通用配置(const 配置格式& fmt);
        //读取自定义模块路由表（route/custom_<模块名>.json，缓存到 自定义路由表）
        bool 读取自定义路由(const std::string& module, nlohmann::json& out);
        //写入自定义模块路由表（route/custom_<模块名>.json）
        bool 写入自定义路由(const std::string& module, const nlohmann::json& data);
        //按模块名生成通用配置路径（config/custom/<模块名>/<条目名>.json）
        std::string 生成通用配置路径(const std::string& module, const std::string& name) const;
    };
}
