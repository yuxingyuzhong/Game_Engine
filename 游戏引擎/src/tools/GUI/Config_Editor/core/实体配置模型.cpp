#include "src/tools/GUI/Config_Editor/实体配置模型.h"
//获取引擎环境（定位 exe 目录，与 Config_Loader::base_dir_get 保持一致）
#include "src/tools/Non_GUI/Engine_Env/引擎环境.h"

//引擎命名空间
namespace engine
{
    //========================================================================
    // 内部工具（UTF-8 路径转换）
    //========================================================================
    namespace
    {
        //std::string (UTF-8) → std::filesystem::path
        std::filesystem::path utf8_path(const std::string& s)
        {
            std::u8string u8s;
            u8s.reserve(s.size());
            for (char c : s)
                u8s.push_back(static_cast<char8_t>(c));
            return std::filesystem::path(u8s);
        }

        //std::filesystem::path → std::string (UTF-8)
        std::string path_utf8(const std::filesystem::path& p)
        {
            std::u8string u8s = p.u8string();
            std::string out;
            out.reserve(u8s.size());
            for (char8_t c : u8s)
                out.push_back(static_cast<char>(c));
            return out;
        }
    }

    //========================================================================
    // 实体配置仓库
    //========================================================================

    //构造函数：定位资源目录
    实体配置仓库::实体配置仓库()
    {
        //资源基目录 = exe 所在目录 / assets（与 Config_Loader::base_dir_get 一致）
        assets_dir = Engine_Env::exe_dir_get() / "assets";
        //路由文件路径
        route_path = assets_dir / "config" / "route" / "entity.json";
        //实体配置文件目录
        entities_dir = assets_dir / "config" / "entities";
        //属性槽路由文件路径
        property_route_path = assets_dir / "config" / "route" / "property.json";
        //属性槽配置文件目录
        property_dir = assets_dir / "config" / "property";
        //配置格式定义目录
        format_dir = assets_dir / "config" / "format";
    }

    //加载全部实体配置
    bool 实体配置仓库::加载()
    {
        //清空旧数据
        实体集合.clear();
        route_json = nlohmann::json();
        route_loaded = false;
        属性槽集合.clear();
        property_route_json = nlohmann::json();
        property_route_loaded = false;

        //读取路由表
        if (!读取路由())
            return false;

        //遍历路由条目，读取每个实体配置
        for (const auto& route_data : route_json)
        {
            //字段存在性与类型检查
            if (!route_data.is_object())
                continue;
            if (!route_data.contains("config_path") || !route_data["config_path"].is_string())
                continue;

            //获取配置相对路径
            std::string config_path = route_data["config_path"];
            //构建绝对路径（安全校验：必须以 config/ 开头，禁止 .. 跳转）
            if (config_path.rfind("config/", 0) != 0)
                continue;
            if (config_path.find("..") != std::string::npos)
                continue;

            std::filesystem::path absolute_path = assets_dir / utf8_path(config_path);
            if (!std::filesystem::is_regular_file(absolute_path))
                continue;

            //读取实体 JSON
            nlohmann::json data;
            try
            {
                std::ifstream file(absolute_path);
                if (!file.is_open())
                    continue;
                file >> data;
            }
            catch (const std::exception&)
            {
                continue;
            }

            //解析为数据模型
            实体配置 cfg;
            std::string parse_error;
            if (!解析实体配置(data, cfg, parse_error))
                continue;

            //记录来源路径（用于保存时写回原文件）
            cfg.config_path = config_path;
            实体集合.push_back(std::move(cfg));
        }

        // —— 加载属性槽配置（独立于实体配置，config/property/）——
        //属性槽路由缺失不算致命错误：属性槽配置可后续在编辑器内新建
        if (读取属性槽路由())
        {
            for (const auto& route_data : property_route_json)
            {
                if (!route_data.is_object())
                    continue;
                if (!route_data.contains("config_path") || !route_data["config_path"].is_string())
                    continue;

                std::string config_path = route_data["config_path"];
                if (config_path.rfind("config/", 0) != 0)
                    continue;
                if (config_path.find("..") != std::string::npos)
                    continue;

                std::filesystem::path absolute_path = assets_dir / utf8_path(config_path);
                if (!std::filesystem::is_regular_file(absolute_path))
                    continue;

                nlohmann::json data;
                try
                {
                    std::ifstream file(absolute_path);
                    if (!file.is_open())
                        continue;
                    file >> data;
                }
                catch (const std::exception&)
                {
                    continue;
                }

                属性槽配置 prop;
                std::string parse_error;
                if (!解析属性槽配置(data, prop, parse_error))
                    continue;

                prop.config_path = config_path;
                属性槽集合.push_back(std::move(prop));
            }
        }

        // —— 加载配置格式定义（内置种子 + 磁盘格式文件）——
        格式集合.clear();
        通用配置集合.clear();
        自定义路由表.clear();
        加载格式();

        // —— 加载自定义模块的通用配置 ——
        //遍历所有非内置格式（自定义模块），读取各自路由 + 配置 JSON
        for (const auto& fmt : 格式集合)
        {
            if (fmt.内置)
                continue;
            加载模块通用配置(fmt);
        }

        return true;
    }

    //获取全部实体配置
    const std::vector<实体配置>& 实体配置仓库::获取全部() const
    {
        return 实体集合;
    }

    //获取全部实体配置（可修改）
    std::vector<实体配置>& 实体配置仓库::获取全部()
    {
        return 实体集合;
    }

    //按类型查找实体配置
    实体配置* 实体配置仓库::查找类型(const std::string& type)
    {
        for (auto& cfg : 实体集合)
            if (cfg.type == type)
                return &cfg;
        return nullptr;
    }

    //获取资源目录
    const std::filesystem::path& 实体配置仓库::资源目录() const
    {
        return assets_dir;
    }

    //获取脚本相对路径列表
    std::vector<std::string> 实体配置仓库::获取脚本列表() const
    {
        std::vector<std::string> result;
        std::filesystem::path scripts_dir = assets_dir / "scripts";
        if (!std::filesystem::is_directory(scripts_dir))
            return result;

        //递归扫描 scripts 目录下的所有 .lua 文件
        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(scripts_dir, ec);
            it != std::filesystem::recursive_directory_iterator(); it.increment(ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }
            if (!it->is_regular_file(ec))
                continue;
            if (it->path().extension() != ".lua")
                continue;

            //转换为相对 assets/ 的路径（正斜杠，UTF-8）
            std::string relative = path_utf8(it->path().lexically_relative(assets_dir));
            std::replace(relative.begin(), relative.end(), '\\', '/');
            result.push_back(std::move(relative));
        }

        //按字典序排序，方便 UI 浏览
        std::sort(result.begin(), result.end());
        return result;
    }

    //校验配置
    bool 实体配置仓库::校验(const 实体配置& cfg,
        std::vector<std::string>& errors,
        std::vector<std::string>& warnings) const
    {
        errors.clear();
        warnings.clear();

        // —— 硬性检查（复刻 Config_Checker::field_check 逻辑） ——

        //字段 "type"：string 且非空
        if (cfg.type.empty())
            errors.push_back("字段 type 不能为空（引擎 field_check 要求）");

        //字段 "decision_load_path"：string 且非空（Entity_Manager 新格式必填）
        if (cfg.decision_load_path.empty())
            errors.push_back("字段 decision_load_path 不能为空（Entity_Manager 现要求该字段，旧配置需补填）");

        //字段 "acls"：vector<string> 且非空
        if (cfg.acls.empty())
            errors.push_back("字段 acls 不能为空（引擎 field_check 对容器有非空检查）");
        for (const auto& acl : cfg.acls)
            if (acl.empty())
                errors.push_back("字段 acls 中存在空字符串条目");

        //字段 "needed_events"：vector<pair<string,string>> 且非空
        if (cfg.needed_events.empty())
            errors.push_back("字段 needed_events 不能为空（引擎 field_check 对容器有非空检查）");
        for (const auto& evt : cfg.needed_events)
        {
            if (evt.first.empty())
                errors.push_back("needed_events 中存在空的事件分类（category）");
            if (evt.second.empty())
                errors.push_back("needed_events 中存在空的事件标签（tag）");
        }

        // —— 软性检查（警告） ——

        //决策树脚本存在性检查
        if (!cfg.decision_load_path.empty())
        {
            std::filesystem::path script_path = assets_dir / utf8_path(cfg.decision_load_path);
            if (!std::filesystem::is_regular_file(script_path))
                warnings.push_back("决策树脚本不存在：" + cfg.decision_load_path +
                    "（Dynamic_Entity 加载时会调用 load_file，路径错误将导致实体行为初始化失败）");
        }

        //acls 引用类型存在性检查
        for (const auto& acl : cfg.acls)
        {
            if (acl.empty())
                continue;
            bool found = false;
            for (const auto& other : 实体集合)
                if (other.type == acl)
                {
                    found = true;
                    break;
                }
            //若引用的类型尚未登记（或正指向自身），仅提示
            if (!found && acl != cfg.type)
                warnings.push_back("acls 引用的实体类型未登记：" + acl);
        }

        //acls 重复条目检查
        for (size_t i = 0; i < cfg.acls.size(); ++i)
            for (size_t j = i + 1; j < cfg.acls.size(); ++j)
                if (cfg.acls[i] == cfg.acls[j])
                {
                    warnings.push_back("acls 存在重复条目：" + cfg.acls[i]);
                    break;
                }

        //needed_events 重复检查
        for (size_t i = 0; i < cfg.needed_events.size(); ++i)
            for (size_t j = i + 1; j < cfg.needed_events.size(); ++j)
                if (cfg.needed_events[i] == cfg.needed_events[j])
                {
                    warnings.push_back("needed_events 存在重复条目：[" +
                        cfg.needed_events[i].first + "," + cfg.needed_events[i].second + "]");
                    break;
                }

        //返回是否存在致命错误
        return errors.empty();
    }

    //保存单个实体配置
    bool 实体配置仓库::保存实体(实体配置& cfg, std::string& error)
    {
        //type 不得为空
        if (cfg.type.empty())
        {
            error = "实体类型（type）不能为空";
            return false;
        }

        //确定目标路径：type 变更时迁移文件
        std::string target_path = 生成配置路径(cfg.type);
        std::filesystem::path absolute_target = assets_dir / utf8_path(target_path);

        //若原路径存在且与目标路径不同，迁移文件（重命名）
        if (!cfg.config_path.empty() && cfg.config_path != target_path)
        {
            std::filesystem::path absolute_old = assets_dir / utf8_path(cfg.config_path);
            if (std::filesystem::exists(absolute_old))
            {
                std::error_code ec;
                std::filesystem::rename(absolute_old, absolute_target, ec);
                if (ec)
                {
                    error = "实体文件迁移失败：" + cfg.config_path + " → " + target_path;
                    return false;
                }
            }
        }
        //若为全新实体（无原路径），直接使用目标路径
        cfg.config_path = target_path;

        //序列化并写入实体 JSON
        nlohmann::json data = 序列化实体配置(cfg);
        try
        {
            std::filesystem::create_directories(absolute_target.parent_path());
            std::ofstream file(absolute_target, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                error = "实体文件打开失败：" + cfg.config_path;
                return false;
            }
            file << data.dump(2);
        }
        catch (const std::exception& e)
        {
            error = std::string("实体文件写入异常：") + e.what();
            return false;
        }

        //同步路由表（条目存在则更新 config_path，不存在则追加）
        if (route_json.is_array())
        {
            bool found = false;
            for (auto& route_data : route_json)
            {
                if (route_data.is_object() &&
                    route_data.value("config_path", std::string()) == cfg.config_path)
                {
                    route_data["config_path"] = cfg.config_path;
                    found = true;
                    break;
                }
            }
            if (!found)
                route_json.push_back({ {"module", "Entity_Mangaer"}, {"config_path", cfg.config_path} });
        }
        if (!写入路由())
        {
            error = "路由表写入失败：" + path_utf8(route_path);
            return false;
        }

        return true;
    }

    //新建实体配置
    bool 实体配置仓库::新建实体(实体配置& cfg, std::string& error)
    {
        //type 不得为空
        if (cfg.type.empty())
        {
            error = "实体类型（type）不能为空";
            return false;
        }
        //type 不得重复
        if (查找类型(cfg.type) != nullptr)
        {
            error = "实体类型已存在：" + cfg.type;
            return false;
        }

        //生成配置路径并写入文件
        cfg.config_path = 生成配置路径(cfg.type);
        nlohmann::json data = 序列化实体配置(cfg);
        std::filesystem::path absolute_path = assets_dir / utf8_path(cfg.config_path);
        try
        {
            std::filesystem::create_directories(absolute_path.parent_path());
            std::ofstream file(absolute_path, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                error = "实体文件打开失败：" + cfg.config_path;
                return false;
            }
            file << data.dump(2);
        }
        catch (const std::exception& e)
        {
            error = std::string("实体文件写入异常：") + e.what();
            return false;
        }

        //追加路由条目
        if (route_json.is_array())
            route_json.push_back({ {"module", "Entity_Mangaer"}, {"config_path", cfg.config_path} });
        if (!写入路由())
        {
            error = "路由表写入失败：" + path_utf8(route_path);
            return false;
        }

        //加入内存集合
        实体集合.push_back(cfg);
        return true;
    }

    //删除实体配置（仅移除路由条目，实体文件保留避免误删）
    bool 实体配置仓库::删除实体(const std::string& type, std::string& error)
    {
        //查找目标实体
        实体配置* target = 查找类型(type);
        if (target == nullptr)
        {
            error = "实体类型不存在：" + type;
            return false;
        }
        std::string config_path = target->config_path;

        //从路由表移除条目
        if (route_json.is_array())
        {
            route_json.erase(
                std::remove_if(route_json.begin(), route_json.end(),
                    [&config_path](const nlohmann::json& item)
                    {
                        return item.is_object() &&
                            item.value("config_path", std::string()) == config_path;
                    }),
                route_json.end());
        }
        if (!写入路由())
        {
            error = "路由表写入失败：" + path_utf8(route_path);
            return false;
        }

        //从内存集合移除
        实体集合.erase(
            std::remove_if(实体集合.begin(), 实体集合.end(),
                [&type](const 实体配置& cfg) { return cfg.type == type; }),
            实体集合.end());

        return true;
    }

    //========================================================================
    // 属性槽配置公开实现
    //========================================================================

    //获取全部属性槽配置（只读）
    const std::vector<属性槽配置>& 实体配置仓库::获取属性槽全部() const
    {
        return 属性槽集合;
    }

    //获取全部属性槽配置（可修改）
    std::vector<属性槽配置>& 实体配置仓库::获取属性槽全部()
    {
        return 属性槽集合;
    }

    //按类型查找属性槽配置
    属性槽配置* 实体配置仓库::查找属性槽(const std::string& type)
    {
        for (auto& cfg : 属性槽集合)
            if (cfg.type == type)
                return &cfg;
        return nullptr;
    }

    //校验属性槽配置
    bool 实体配置仓库::校验属性槽(const 属性槽配置& cfg,
        std::vector<std::string>& errors,
        std::vector<std::string>& warnings) const
    {
        errors.clear();
        warnings.clear();

        //硬性检查
        if (cfg.type.empty())
            errors.push_back("属性槽配置：字段 type 不能为空（Property_Manager field_check 要求）");
        if (cfg.initialize_path.empty())
            errors.push_back("属性槽配置：字段 initialize_path 不能为空（Property_Manager field_check 要求）");

        //软性检查：初始化脚本存在性
        if (!cfg.initialize_path.empty())
        {
            std::filesystem::path script_path = assets_dir / utf8_path(cfg.initialize_path);
            if (!std::filesystem::is_regular_file(script_path))
                warnings.push_back("属性槽初始化脚本不存在：" + cfg.initialize_path +
                    "（Property_Manager 加载时会调用 load_file，路径错误将导致属性槽初始化失败）");
        }

        return errors.empty();
    }

    //保存单个属性槽配置
    bool 实体配置仓库::保存属性槽(属性槽配置& cfg, std::string& error)
    {
        if (cfg.type.empty())
        {
            error = "属性槽配置：实体类型（type）不能为空";
            return false;
        }

        //确定目标路径（type 变更时迁移文件）
        std::string target_path = 生成属性槽配置路径(cfg.type);
        std::filesystem::path absolute_target = assets_dir / utf8_path(target_path);

        if (!cfg.config_path.empty() && cfg.config_path != target_path)
        {
            std::filesystem::path absolute_old = assets_dir / utf8_path(cfg.config_path);
            if (std::filesystem::exists(absolute_old))
            {
                std::error_code ec;
                std::filesystem::rename(absolute_old, absolute_target, ec);
                if (ec)
                {
                    error = "属性槽文件迁移失败：" + cfg.config_path + " → " + target_path;
                    return false;
                }
            }
        }
        cfg.config_path = target_path;

        //序列化并写入属性槽 JSON
        nlohmann::json data = 序列化属性槽配置(cfg);
        try
        {
            std::filesystem::create_directories(absolute_target.parent_path());
            std::ofstream file(absolute_target, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                error = "属性槽文件打开失败：" + cfg.config_path;
                return false;
            }
            file << data.dump(2);
        }
        catch (const std::exception& e)
        {
            error = std::string("属性槽文件写入异常：") + e.what();
            return false;
        }

        //同步属性槽路由表
        if (property_route_json.is_array())
        {
            bool found = false;
            for (auto& route_data : property_route_json)
            {
                if (route_data.is_object() &&
                    route_data.value("config_path", std::string()) == cfg.config_path)
                {
                    route_data["config_path"] = cfg.config_path;
                    found = true;
                    break;
                }
            }
            if (!found)
                property_route_json.push_back({ {"module", "Property_Manager"}, {"config_path", cfg.config_path} });
        }
        if (!写入属性槽路由())
        {
            error = "属性槽路由表写入失败：" + path_utf8(property_route_path);
            return false;
        }

        return true;
    }

    //新建属性槽配置
    bool 实体配置仓库::新建属性槽(属性槽配置& cfg, std::string& error)
    {
        if (cfg.type.empty())
        {
            error = "属性槽配置：实体类型（type）不能为空";
            return false;
        }
        if (查找属性槽(cfg.type) != nullptr)
        {
            error = "属性槽配置已存在：" + cfg.type;
            return false;
        }

        cfg.config_path = 生成属性槽配置路径(cfg.type);
        nlohmann::json data = 序列化属性槽配置(cfg);
        std::filesystem::path absolute_path = assets_dir / utf8_path(cfg.config_path);
        try
        {
            std::filesystem::create_directories(absolute_path.parent_path());
            std::ofstream file(absolute_path, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                error = "属性槽文件打开失败：" + cfg.config_path;
                return false;
            }
            file << data.dump(2);
        }
        catch (const std::exception& e)
        {
            error = std::string("属性槽文件写入异常：") + e.what();
            return false;
        }

        if (property_route_json.is_array())
            property_route_json.push_back({ {"module", "Property_Manager"}, {"config_path", cfg.config_path} });
        if (!写入属性槽路由())
        {
            error = "属性槽路由表写入失败：" + path_utf8(property_route_path);
            return false;
        }

        属性槽集合.push_back(cfg);
        return true;
    }

    //删除属性槽配置（仅移除路由条目，属性槽文件保留）
    bool 实体配置仓库::删除属性槽(const std::string& type, std::string& error)
    {
        属性槽配置* target = 查找属性槽(type);
        if (target == nullptr)
        {
            error = "属性槽配置不存在：" + type;
            return false;
        }
        std::string config_path = target->config_path;

        if (property_route_json.is_array())
        {
            property_route_json.erase(
                std::remove_if(property_route_json.begin(), property_route_json.end(),
                    [&config_path](const nlohmann::json& item)
                    {
                        return item.is_object() &&
                            item.value("config_path", std::string()) == config_path;
                    }),
                property_route_json.end());
        }
        if (!写入属性槽路由())
        {
            error = "属性槽路由表写入失败：" + path_utf8(property_route_path);
            return false;
        }

        属性槽集合.erase(
            std::remove_if(属性槽集合.begin(), 属性槽集合.end(),
                [&type](const 属性槽配置& cfg) { return cfg.type == type; }),
            属性槽集合.end());

        return true;
    }

    //========================================================================
    // 私有实现
    //========================================================================

    //读取路由表
    bool 实体配置仓库::读取路由()
    {
        if (!std::filesystem::is_regular_file(route_path))
            return false;

        try
        {
            std::ifstream file(route_path);
            if (!file.is_open())
                return false;
            file >> route_json;
        }
        catch (const std::exception&)
        {
            route_json = nlohmann::json();
            return false;
        }

        //路由表必须是 JSON 数组
        if (!route_json.is_array())
        {
            route_json = nlohmann::json();
            return false;
        }

        route_loaded = true;
        return true;
    }

    //写入路由表
    bool 实体配置仓库::写入路由()
    {
        try
        {
            std::filesystem::create_directories(route_path.parent_path());
            std::ofstream file(route_path, std::ios::out | std::ios::trunc);
            if (!file.is_open())
                return false;
            file << route_json.dump(2);
        }
        catch (const std::exception&)
        {
            return false;
        }
        return true;
    }

    //解析单个实体 JSON（兼容旧格式）
    bool 实体配置仓库::解析实体配置(const nlohmann::json& data, 实体配置& out, std::string& error)
    {
        if (!data.is_object())
        {
            error = "配置内容非 JSON 对象";
            return false;
        }

        // —— type ——
        if (data.contains("type") && data["type"].is_string())
            out.type = data["type"];
        else
            out.type.clear();

        // —— decision_load_path ——（新格式字段；旧配置缺失时保持为空，由校验提示补填）
        if (data.contains("decision_load_path") && data["decision_load_path"].is_string())
            out.decision_load_path = data["decision_load_path"];
        else
            out.decision_load_path.clear();

        // —— acls ——
        out.acls.clear();
        if (data.contains("acls"))
        {
            const auto& acls_field = data["acls"];
            //新格式：字符串数组
            if (acls_field.is_array())
            {
                for (const auto& item : acls_field)
                    if (item.is_string())
                        out.acls.push_back(item.get<std::string>());
            }
            //旧格式：对象 { master, minion_set }
            else if (acls_field.is_object())
            {
                if (acls_field.contains("minion_set") && acls_field["minion_set"].is_array())
                {
                    for (const auto& item : acls_field["minion_set"])
                        if (item.is_string())
                            out.acls.push_back(item.get<std::string>());
                }
                //若对象仅含 master（字符串），将 master 作为唯一从属
                if (out.acls.empty() && acls_field.contains("master") && acls_field["master"].is_string())
                    out.acls.push_back(acls_field["master"].get<std::string>());
            }
        }

        // —— needed_events ——
        out.needed_events.clear();
        if (data.contains("needed_events") && data["needed_events"].is_array())
        {
            for (const auto& item : data["needed_events"])
            {
                //新格式：两元素数组 ["分类","标签"]
                if (item.is_array() && item.size() >= 2 &&
                    item[0].is_string() && item[1].is_string())
                {
                    out.needed_events.emplace_back(item[0].get<std::string>(),
                        item[1].get<std::string>());
                }
                //旧格式：对象 { category, tag }
                else if (item.is_object() &&
                    item.contains("category") && item["category"].is_string() &&
                    item.contains("tag") && item["tag"].is_string())
                {
                    out.needed_events.emplace_back(item["category"].get<std::string>(),
                        item["tag"].get<std::string>());
                }
            }
        }

        return true;
    }

    //序列化实体配置为标准 JSON
    nlohmann::json 实体配置仓库::序列化实体配置(const 实体配置& cfg) const
    {
        nlohmann::json data;
        data["type"] = cfg.type;
        data["decision_load_path"] = cfg.decision_load_path;

        //acls：字符串数组
        data["acls"] = nlohmann::json::array();
        for (const auto& acl : cfg.acls)
            data["acls"].push_back(acl);

        //needed_events：两元素数组的数组（引擎期望 vector<pair<string,string>>）
        data["needed_events"] = nlohmann::json::array();
        for (const auto& evt : cfg.needed_events)
            data["needed_events"].push_back({ evt.first, evt.second });

        return data;
    }

    //按类型生成默认配置路径
    std::string 实体配置仓库::生成配置路径(const std::string& type) const
    {
        return "config/entities/" + 文件名清理(type) + ".json";
    }

    //字符串清理（剔除路径非法字符）
    std::string 实体配置仓库::文件名清理(const std::string& name)
    {
        std::string cleaned = name;
        for (auto& ch : cleaned)
        {
            switch (ch)
            {
            case '\\': case '/': case ':': case '*':
            case '?': case '"': case '<': case '>':
            case '|':
                ch = '_';
                break;
            default:
                break;
            }
        }
        //去除首尾空白
        while (!cleaned.empty() && (cleaned.front() == ' ' || cleaned.front() == '\t'))
            cleaned.erase(cleaned.begin());
        while (!cleaned.empty() && (cleaned.back() == ' ' || cleaned.back() == '\t'))
            cleaned.pop_back();
        if (cleaned.empty())
            cleaned = "未命名实体";
        return cleaned;
    }

    //读取属性槽路由表
    bool 实体配置仓库::读取属性槽路由()
    {
        if (!std::filesystem::is_regular_file(property_route_path))
            return false;

        try
        {
            std::ifstream file(property_route_path);
            if (!file.is_open())
                return false;
            file >> property_route_json;
        }
        catch (const std::exception&)
        {
            property_route_json = nlohmann::json();
            return false;
        }

        if (!property_route_json.is_array())
        {
            property_route_json = nlohmann::json();
            return false;
        }

        property_route_loaded = true;
        return true;
    }

    //写入属性槽路由表
    bool 实体配置仓库::写入属性槽路由()
    {
        try
        {
            std::filesystem::create_directories(property_route_path.parent_path());
            std::ofstream file(property_route_path, std::ios::out | std::ios::trunc);
            if (!file.is_open())
                return false;
            file << property_route_json.dump(2);
        }
        catch (const std::exception&)
        {
            return false;
        }
        return true;
    }

    //解析单个属性槽 JSON
    bool 实体配置仓库::解析属性槽配置(const nlohmann::json& data, 属性槽配置& out, std::string& error)
    {
        if (!data.is_object())
        {
            error = "属性槽配置内容非 JSON 对象";
            return false;
        }

        if (data.contains("type") && data["type"].is_string())
            out.type = data["type"];
        else
            out.type.clear();

        if (data.contains("initialize_path") && data["initialize_path"].is_string())
            out.initialize_path = data["initialize_path"];
        else
            out.initialize_path.clear();

        return true;
    }

    //序列化属性槽配置为标准 JSON
    nlohmann::json 实体配置仓库::序列化属性槽配置(const 属性槽配置& cfg) const
    {
        nlohmann::json data;
        data["type"] = cfg.type;
        data["initialize_path"] = cfg.initialize_path;
        return data;
    }

    //按类型生成属性槽默认配置路径
    std::string 实体配置仓库::生成属性槽配置路径(const std::string& type) const
    {
        return "config/property/" + 文件名清理(type) + ".json";
    }

    //========================================================================
    // 配置格式系统实现
    //========================================================================

    //获取全部配置格式
    const std::vector<配置格式>& 实体配置仓库::获取格式全部() const
    {
        return 格式集合;
    }

    //获取全部配置格式（可修改）
    std::vector<配置格式>& 实体配置仓库::获取格式全部()
    {
        return 格式集合;
    }

    //按模块名查找配置格式
    配置格式* 实体配置仓库::查找格式(const std::string& module)
    {
        for (auto& fmt : 格式集合)
            if (fmt.模块名 == module)
                return &fmt;
        return nullptr;
    }

    //字段类型 → 显示名
    const char* 实体配置仓库::字段类型名称(配置字段类型 type)
    {
        switch (type)
        {
        case 配置字段类型::文本:       return "文本";
        case 配置字段类型::脚本路径:   return "脚本路径";
        case 配置字段类型::文本列表:   return "文本列表";
        case 配置字段类型::事件对列表: return "事件对列表";
        case 配置字段类型::整数:       return "整数";
        case 配置字段类型::浮点数:     return "浮点数";
        case 配置字段类型::布尔:       return "布尔";
        default:                       return "文本";
        }
    }

    //字段类型 → JSON 序列化键名
    const char* 实体配置仓库::字段类型键(配置字段类型 type)
    {
        switch (type)
        {
        case 配置字段类型::文本:       return "string";
        case 配置字段类型::脚本路径:   return "script";
        case 配置字段类型::文本列表:   return "string_list";
        case 配置字段类型::事件对列表: return "pair_list";
        case 配置字段类型::整数:       return "int";
        case 配置字段类型::浮点数:     return "float";
        case 配置字段类型::布尔:       return "bool";
        default:                       return "string";
        }
    }

    //按 JSON 键名解析字段类型
    配置字段类型 实体配置仓库::解析字段类型(const std::string& key)
    {
        if (key == "script")       return 配置字段类型::脚本路径;
        if (key == "string_list")  return 配置字段类型::文本列表;
        if (key == "pair_list")    return 配置字段类型::事件对列表;
        if (key == "int")          return 配置字段类型::整数;
        if (key == "float")        return 配置字段类型::浮点数;
        if (key == "bool")         return 配置字段类型::布尔;
        return 配置字段类型::文本;
    }

    //确保内置格式种子存在（Entity_Manager / Property_Manager）
    void 实体配置仓库::确保内置格式()
    {
        //内置格式定义（与引擎 Entity_Manager / Property_Manager 期望的字段一致）
        const 配置格式 内置格式[] =
        {
            {
                "Entity_Manager",
                "entities",
                "entity.json",
                true,   //内置
                {
                    { "type",              "type（实体类型）",          配置字段类型::文本,     true,  "实体类型标识，保存后配置文件会自动迁移" },
                    { "decision_load_path", "decision_load_path（决策树行为脚本）", 配置字段类型::脚本路径, true,  "决策树行为脚本路径，相对 assets/ 目录，Entity_Manager 使用" },
                    { "acls",              "acls（从属权限列表）",      配置字段类型::文本列表, true,  "允许从属的实体类型列表（master 为实体类型自身）" },
                    { "needed_events",     "needed_events（订阅事件列表）", 配置字段类型::事件对列表, true,  "订阅事件列表，每项 = [分类, 标签]，如 [Entity, Request]" },
                }
            },
            {
                "Property_Manager",
                "property",
                "property.json",
                true,   //内置
                {
                    { "type",            "type（实体类型）",      配置字段类型::文本,     true,  "实体类型标识，与实体配置的 type 一致" },
                    { "initialize_path", "initialize_path（属性槽初始化脚本）", 配置字段类型::脚本路径, true,  "属性槽初始化 Lua 脚本路径，相对 assets/ 目录，Property_Manager 使用" },
                }
            },
        };

        //逐个检查：格式文件缺失则写入种子
        for (const auto& fmt : 内置格式)
        {
            std::filesystem::path file_path = format_dir / (文件名清理(fmt.模块名) + ".json");
            if (!std::filesystem::is_regular_file(file_path))
                写入格式文件(fmt);
        }
    }

    //读取单个格式定义文件
    bool 实体配置仓库::读取格式文件(const std::filesystem::path& path, 配置格式& out)
    {
        try
        {
            nlohmann::json data;
            std::ifstream file(path);
            if (!file.is_open())
                return false;
            file >> data;

            if (!data.is_object())
                return false;
            out = 配置格式();
            out.模块名 = data.value("module", std::string());
            out.配置目录 = data.value("dir", std::string());
            out.路由文件名 = data.value("route", std::string());
            out.内置 = data.value("builtin", false);

            //解析字段列表
            if (data.contains("fields") && data["fields"].is_array())
            {
                for (const auto& f : data["fields"])
                {
                    if (!f.is_object())
                        continue;
                    配置字段定义 field;
                    field.字段名 = f.value("name", std::string());
                    field.显示名 = f.value("display", std::string());
                    field.类型 = 解析字段类型(f.value("type", std::string("string")));
                    field.必填 = f.value("required", true);
                    field.说明 = f.value("desc", std::string());
                    if (!field.字段名.empty())
                        out.字段.push_back(std::move(field));
                }
            }
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    //写入单个格式定义文件
    bool 实体配置仓库::写入格式文件(const 配置格式& fmt)
    {
        try
        {
            nlohmann::json data;
            data["module"] = fmt.模块名;
            data["dir"] = fmt.配置目录;
            data["route"] = fmt.路由文件名;
            data["builtin"] = fmt.内置;
            data["fields"] = nlohmann::json::array();
            for (const auto& f : fmt.字段)
            {
                nlohmann::json field;
                field["name"] = f.字段名;
                field["display"] = f.显示名;
                field["type"] = 字段类型键(f.类型);
                field["required"] = f.必填;
                field["desc"] = f.说明;
                data["fields"].push_back(std::move(field));
            }

            std::filesystem::create_directories(format_dir);
            std::ofstream file(format_dir / (文件名清理(fmt.模块名) + ".json"),
                std::ios::out | std::ios::trunc);
            if (!file.is_open())
                return false;
            file << data.dump(2);
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    //加载全部格式定义
    bool 实体配置仓库::加载格式()
    {
        //首次运行：确保内置格式种子文件存在
        确保内置格式();

        //读取 format/ 目录下全部格式文件（按文件名排序，保证顺序稳定）
        std::vector<std::filesystem::path> files;
        std::error_code ec;
        if (std::filesystem::is_directory(format_dir))
        {
            for (auto it = std::filesystem::directory_iterator(format_dir, ec);
                it != std::filesystem::directory_iterator(); it.increment(ec))
            {
                if (ec)
                {
                    ec.clear();
                    continue;
                }
                if (it->is_regular_file(ec) && it->path().extension() == ".json")
                    files.push_back(it->path());
            }
        }
        std::sort(files.begin(), files.end());

        for (const auto& path : files)
        {
            配置格式 fmt;
            if (读取格式文件(path, fmt) && !fmt.模块名.empty())
                格式集合.push_back(std::move(fmt));
        }

        return !格式集合.empty();
    }

    //保存格式定义
    bool 实体配置仓库::保存格式(const 配置格式& fmt, std::string& error)
    {
        //内置格式拒绝覆盖（防止误改引擎要求的格式）
        if (fmt.内置)
        {
            error = "内置格式（" + fmt.模块名 + "）不可编辑保存";
            return false;
        }
        if (fmt.模块名.empty())
        {
            error = "模块名不能为空";
            return false;
        }
        if (fmt.配置目录.empty() || fmt.路由文件名.empty())
        {
            error = "配置目录与路由文件名不能为空";
            return false;
        }

        //写入格式文件
        if (!写入格式文件(fmt))
        {
            error = "格式文件写入失败：" + fmt.模块名;
            return false;
        }

        //更新内存集合：已存在则替换，否则追加
        配置格式* existing = 查找格式(fmt.模块名);
        if (existing != nullptr)
            *existing = fmt;
        else
            格式集合.push_back(fmt);

        return true;
    }

    //删除格式定义
    bool 实体配置仓库::删除格式(const std::string& module, std::string& error)
    {
        配置格式* target = 查找格式(module);
        if (target == nullptr)
        {
            error = "格式不存在：" + module;
            return false;
        }
        if (target->内置)
        {
            error = "内置格式不可删除：" + module;
            return false;
        }

        //删除格式文件
        std::error_code ec;
        std::filesystem::remove(format_dir / (文件名清理(module) + ".json"), ec);

        //从内存集合移除
        格式集合.erase(
            std::remove_if(格式集合.begin(), 格式集合.end(),
                [&module](const 配置格式& fmt) { return fmt.模块名 == module; }),
            格式集合.end());

        //同时移除该模块的通用配置与路由缓存
        通用配置集合.erase(
            std::remove_if(通用配置集合.begin(), 通用配置集合.end(),
                [&module](const 通用配置& cfg) { return cfg.模块名 == module; }),
            通用配置集合.end());
        自定义路由表.erase(module);

        return true;
    }

    //========================================================================
    // 通用配置实现（自定义模块）
    //========================================================================

    //获取全部通用配置
    const std::vector<通用配置>& 实体配置仓库::获取通用配置全部() const
    {
        return 通用配置集合;
    }

    //获取全部通用配置（可修改）
    std::vector<通用配置>& 实体配置仓库::获取通用配置全部()
    {
        return 通用配置集合;
    }

    //按模块名 + 条目名查找通用配置
    通用配置* 实体配置仓库::查找通用配置(const std::string& module, const std::string& name)
    {
        for (auto& cfg : 通用配置集合)
            if (cfg.模块名 == module && cfg.条目名 == name)
                return &cfg;
        return nullptr;
    }

    //按格式定义生成默认字段值
    nlohmann::json 实体配置仓库::生成默认字段值(const 配置格式& fmt)
    {
        nlohmann::json data = nlohmann::json::object();
        for (const auto& f : fmt.字段)
        {
            switch (f.类型)
            {
            case 配置字段类型::文本:
            case 配置字段类型::脚本路径:
                data[f.字段名] = "";
                break;
            case 配置字段类型::文本列表:
                data[f.字段名] = nlohmann::json::array();
                break;
            case 配置字段类型::事件对列表:
                data[f.字段名] = nlohmann::json::array();
                break;
            case 配置字段类型::整数:
                data[f.字段名] = 0;
                break;
            case 配置字段类型::浮点数:
                data[f.字段名] = 0.0;
                break;
            case 配置字段类型::布尔:
                data[f.字段名] = false;
                break;
            }
        }
        return data;
    }

    //从字段值中提取条目名
    std::string 实体配置仓库::提取条目名(const nlohmann::json& data)
    {
        if (data.is_object())
        {
            //优先取 type，其次 name，再其次 id
            for (const char* key : { "type", "name", "id" })
            {
                if (data.contains(key) && data[key].is_string())
                    return data[key].get<std::string>();
            }
        }
        return std::string();
    }

    //新建通用配置
    bool 实体配置仓库::新建通用配置(通用配置& cfg, std::string& error)
    {
        if (cfg.模块名.empty())
        {
            error = "模块名不能为空";
            return false;
        }
        if (cfg.条目名.empty())
        {
            error = "条目名不能为空";
            return false;
        }
        if (查找通用配置(cfg.模块名, cfg.条目名) != nullptr)
        {
            error = "配置已存在：" + cfg.模块名 + "/" + cfg.条目名;
            return false;
        }
        配置格式* fmt = 查找格式(cfg.模块名);
        if (fmt == nullptr)
        {
            error = "模块格式不存在：" + cfg.模块名;
            return false;
        }
        if (fmt->内置)
        {
            error = "内置模块请使用专用创建入口：" + cfg.模块名;
            return false;
        }

        //生成配置路径并写入文件
        cfg.config_path = 生成通用配置路径(cfg.模块名, cfg.条目名);
        std::filesystem::path absolute_path = assets_dir / utf8_path(cfg.config_path);
        try
        {
            std::filesystem::create_directories(absolute_path.parent_path());
            std::ofstream file(absolute_path, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                error = "配置文件打开失败：" + cfg.config_path;
                return false;
            }
            file << cfg.字段值.dump(2);
        }
        catch (const std::exception& e)
        {
            error = std::string("配置文件写入异常：") + e.what();
            return false;
        }

        //追加自定义模块路由条目
        auto it = 自定义路由表.find(cfg.模块名);
        if (it == 自定义路由表.end())
        {
            nlohmann::json empty = nlohmann::json::array();
            it = 自定义路由表.emplace(cfg.模块名, std::move(empty)).first;
        }
        if (it->second.is_array())
            it->second.push_back({ {"module", cfg.模块名}, {"config_path", cfg.config_path} });
        if (!写入自定义路由(cfg.模块名, it->second))
        {
            error = "路由表写入失败：custom_" + 文件名清理(cfg.模块名);
            return false;
        }

        //加入内存集合
        通用配置集合.push_back(cfg);
        return true;
    }

    //保存通用配置
    bool 实体配置仓库::保存通用配置(通用配置& cfg, std::string& error)
    {
        if (cfg.模块名.empty())
        {
            error = "模块名不能为空";
            return false;
        }
        if (cfg.条目名.empty())
        {
            error = "条目名不能为空";
            return false;
        }

        //确定目标路径（条目名变更时迁移文件）
        std::string target_path = 生成通用配置路径(cfg.模块名, cfg.条目名);
        std::filesystem::path absolute_target = assets_dir / utf8_path(target_path);

        if (!cfg.config_path.empty() && cfg.config_path != target_path)
        {
            std::filesystem::path absolute_old = assets_dir / utf8_path(cfg.config_path);
            if (std::filesystem::exists(absolute_old))
            {
                std::error_code ec;
                std::filesystem::rename(absolute_old, absolute_target, ec);
                if (ec)
                {
                    error = "配置文件迁移失败：" + cfg.config_path + " → " + target_path;
                    return false;
                }
            }
        }
        cfg.config_path = target_path;

        //写入 JSON
        std::filesystem::path absolute_path = assets_dir / utf8_path(cfg.config_path);
        try
        {
            std::filesystem::create_directories(absolute_path.parent_path());
            std::ofstream file(absolute_path, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                error = "配置文件打开失败：" + cfg.config_path;
                return false;
            }
            file << cfg.字段值.dump(2);
        }
        catch (const std::exception& e)
        {
            error = std::string("配置文件写入异常：") + e.what();
            return false;
        }

        //同步自定义模块路由表
        auto it = 自定义路由表.find(cfg.模块名);
        if (it == 自定义路由表.end())
        {
            nlohmann::json empty = nlohmann::json::array();
            it = 自定义路由表.emplace(cfg.模块名, std::move(empty)).first;
        }
        if (it->second.is_array())
        {
            bool found = false;
            for (auto& route_data : it->second)
            {
                if (route_data.is_object() &&
                    route_data.value("config_path", std::string()) == cfg.config_path)
                {
                    route_data["config_path"] = cfg.config_path;
                    found = true;
                    break;
                }
            }
            if (!found)
                it->second.push_back({ {"module", cfg.模块名}, {"config_path", cfg.config_path} });
        }
        if (!写入自定义路由(cfg.模块名, it->second))
        {
            error = "路由表写入失败：custom_" + 文件名清理(cfg.模块名);
            return false;
        }

        return true;
    }

    //删除通用配置
    bool 实体配置仓库::删除通用配置(const std::string& module, const std::string& name, std::string& error)
    {
        通用配置* target = 查找通用配置(module, name);
        if (target == nullptr)
        {
            error = "配置不存在：" + module + "/" + name;
            return false;
        }
        std::string config_path = target->config_path;

        //从自定义模块路由表移除条目
        auto it = 自定义路由表.find(module);
        if (it != 自定义路由表.end() && it->second.is_array())
        {
            it->second.erase(
                std::remove_if(it->second.begin(), it->second.end(),
                    [&config_path](const nlohmann::json& item)
                    {
                        return item.is_object() &&
                            item.value("config_path", std::string()) == config_path;
                    }),
                it->second.end());
            if (!写入自定义路由(module, it->second))
            {
                error = "路由表写入失败：custom_" + 文件名清理(module);
                return false;
            }
        }

        //从内存集合移除
        通用配置集合.erase(
            std::remove_if(通用配置集合.begin(), 通用配置集合.end(),
                [&module, &name](const 通用配置& cfg)
                {
                    return cfg.模块名 == module && cfg.条目名 == name;
                }),
            通用配置集合.end());

        return true;
    }

    //校验通用配置
    bool 实体配置仓库::校验通用配置(const 配置格式& fmt, const 通用配置& cfg,
        std::vector<std::string>& errors, std::vector<std::string>& warnings) const
    {
        errors.clear();
        warnings.clear();

        for (const auto& f : fmt.字段)
        {
            const bool has = cfg.字段值.contains(f.字段名);
            const auto& value = cfg.字段值[f.字段名];

            switch (f.类型)
            {
            case 配置字段类型::文本:
            case 配置字段类型::脚本路径:
            {
                std::string str = has && value.is_string() ? value.get<std::string>() : std::string();
                if (f.必填 && str.empty())
                    errors.push_back("字段 " + f.字段名 + " 不能为空（必填）");
                //脚本路径存在性软检查
                if (f.类型 == 配置字段类型::脚本路径 && !str.empty())
                {
                    std::filesystem::path script_path = assets_dir / utf8_path(str);
                    if (!std::filesystem::is_regular_file(script_path))
                        warnings.push_back("脚本路径不存在：" + str + "（模块加载时可能失败）");
                }
                break;
            }
            case 配置字段类型::文本列表:
            {
                std::vector<std::string> list;
                if (has && value.is_array())
                {
                    for (const auto& item : value)
                        if (item.is_string())
                            list.push_back(item.get<std::string>());
                }
                if (f.必填 && list.empty())
                    errors.push_back("字段 " + f.字段名 + " 不能为空列表（必填）");
                for (const auto& item : list)
                    if (item.empty())
                        errors.push_back("字段 " + f.字段名 + " 中存在空字符串条目");
                break;
            }
            case 配置字段类型::事件对列表:
            {
                std::vector<std::pair<std::string, std::string>> list;
                if (has && value.is_array())
                {
                    for (const auto& item : value)
                    {
                        if (item.is_array() && item.size() >= 2 &&
                            item[0].is_string() && item[1].is_string())
                            list.emplace_back(item[0].get<std::string>(), item[1].get<std::string>());
                        else if (item.is_object() &&
                            item.contains("category") && item["category"].is_string() &&
                            item.contains("tag") && item["tag"].is_string())
                            list.emplace_back(item["category"].get<std::string>(),
                                item["tag"].get<std::string>());
                    }
                }
                if (f.必填 && list.empty())
                    errors.push_back("字段 " + f.字段名 + " 不能为空列表（必填）");
                for (const auto& item : list)
                {
                    if (item.first.empty())
                        errors.push_back("字段 " + f.字段名 + " 中存在空的事件分类（category）");
                    if (item.second.empty())
                        errors.push_back("字段 " + f.字段名 + " 中存在空的事件标签（tag）");
                }
                break;
            }
            case 配置字段类型::整数:
                if (!has || !value.is_number_integer())
                    errors.push_back("字段 " + f.字段名 + " 必须是整数");
                break;
            case 配置字段类型::浮点数:
                if (!has || !value.is_number())
                    errors.push_back("字段 " + f.字段名 + " 必须是数字");
                break;
            case 配置字段类型::布尔:
                if (!has || !value.is_boolean())
                    errors.push_back("字段 " + f.字段名 + " 必须是布尔值");
                break;
            }
        }

        return errors.empty();
    }

    //读取自定义模块路由表
    bool 实体配置仓库::读取自定义路由(const std::string& module, nlohmann::json& out)
    {
        std::filesystem::path route_path = assets_dir / "config" / "route" /
            ("custom_" + 文件名清理(module) + ".json");
        if (!std::filesystem::is_regular_file(route_path))
            return false;

        try
        {
            std::ifstream file(route_path);
            if (!file.is_open())
                return false;
            file >> out;
        }
        catch (const std::exception&)
        {
            out = nlohmann::json();
            return false;
        }

        return out.is_array();
    }

    //写入自定义模块路由表
    bool 实体配置仓库::写入自定义路由(const std::string& module, const nlohmann::json& data)
    {
        try
        {
            std::filesystem::path route_path = assets_dir / "config" / "route" /
                ("custom_" + 文件名清理(module) + ".json");
            std::filesystem::create_directories(route_path.parent_path());
            std::ofstream file(route_path, std::ios::out | std::ios::trunc);
            if (!file.is_open())
                return false;
            file << data.dump(2);
        }
        catch (const std::exception&)
        {
            return false;
        }
        return true;
    }

    //加载单个自定义模块的全部通用配置
    bool 实体配置仓库::加载模块通用配置(const 配置格式& fmt)
    {
        nlohmann::json route_data;
        if (!读取自定义路由(fmt.模块名, route_data))
        {
            //路由缺失不算致命错误：可后续在编辑器内新建
            自定义路由表[fmt.模块名] = nlohmann::json::array();
            return false;
        }
        自定义路由表[fmt.模块名] = route_data;

        for (const auto& route_item : route_data)
        {
            if (!route_item.is_object())
                continue;
            if (!route_item.contains("config_path") || !route_item["config_path"].is_string())
                continue;

            std::string config_path = route_item["config_path"];
            //安全校验：必须以 config/ 开头，禁止 .. 跳转
            if (config_path.rfind("config/", 0) != 0)
                continue;
            if (config_path.find("..") != std::string::npos)
                continue;

            std::filesystem::path absolute_path = assets_dir / utf8_path(config_path);
            if (!std::filesystem::is_regular_file(absolute_path))
                continue;

            nlohmann::json data;
            try
            {
                std::ifstream file(absolute_path);
                if (!file.is_open())
                    continue;
                file >> data;
            }
            catch (const std::exception&)
            {
                continue;
            }
            if (!data.is_object())
                continue;

            通用配置 cfg;
            cfg.模块名 = fmt.模块名;
            cfg.字段值 = std::move(data);
            cfg.条目名 = 提取条目名(cfg.字段值);
            if (cfg.条目名.empty())
            {
                //无 type/name 字段时用文件名（去扩展名）兜底
                cfg.条目名 = path_utf8(absolute_path.stem());
            }
            cfg.config_path = config_path;
            通用配置集合.push_back(std::move(cfg));
        }

        return true;
    }

    //按模块名生成通用配置路径
    std::string 实体配置仓库::生成通用配置路径(const std::string& module, const std::string& name) const
    {
        return "config/custom/" + 文件名清理(module) + "/" + 文件名清理(name) + ".json";
    }
}
