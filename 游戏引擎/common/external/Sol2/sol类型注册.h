#pragma once
#include "common/前置头文件包含.h"
#include "common/types/事件类型.h"

namespace engine
{
    inline sol::table json_to_table(sol::state_view lua, const nlohmann::json& j) {
        sol::table t = lua.create_table();

        // 将基础 json 值转换为 sol::object
        auto push_value = [&](const nlohmann::json& val) -> sol::object {
            if (val.is_null())           return sol::make_object(lua, sol::nil);
            if (val.is_boolean())        return sol::make_object(lua, val.get<bool>());
            if (val.is_number_integer()) return sol::make_object(lua, val.get<lua_Integer>());
            if (val.is_number_float())   return sol::make_object(lua, val.get<double>());
            if (val.is_string())         return sol::make_object(lua, val.get<std::string>());
            return sol::make_object(lua, sol::nil);
            };

        if (j.is_array()) {
            for (size_t i = 0; i < j.size(); ++i) {
                const nlohmann::json& elem = j[i];
                if (elem.is_object() || elem.is_array()) {
                    t[i + 1] = json_to_table(lua, elem);
                }
                else {
                    t[i + 1] = push_value(elem);
                }
            }
        }
        else if (j.is_object()) {
            // 使用 C++11 兼容的方式遍历对象
            for (nlohmann::json::const_iterator it = j.begin(); it != j.end(); ++it) {
                const std::string& key = it.key();
                const nlohmann::json& value = it.value();
                if (value.is_object() || value.is_array()) {
                    t[key] = json_to_table(lua, value);
                }
                else {
                    t[key] = push_value(value);
                }
            }
        }

        return t;
    }

    inline nlohmann::json table_to_json(sol::state_view lua, const sol::table& t, bool empty_as_array = false) {
        std::vector<sol::object> keys;
        std::vector<sol::object> values;
        t.for_each([&](sol::object key, sol::object value) {
            keys.push_back(key);
            values.push_back(value);
            });

        if (keys.empty()) {
            return empty_as_array ? nlohmann::json::array() : nlohmann::json::object();
        }

        // 检测是否为完整数字序列（数组）
        bool is_array = true;
        std::set<lua_Integer> intKeys;
        // 原范围 for：for (const auto& key : keys)
        // 改为下标访问的三段式循环
        for (size_t idx = 0; idx < keys.size(); ++idx) {
            const sol::object& key = keys[idx];
            if (key.is<lua_Integer>()) {
                lua_Integer k = key.as<lua_Integer>();
                if (k < 1) { is_array = false; break; }
                intKeys.insert(k);
            }
            else if (key.is<double>()) {
                double d = key.as<double>();
                if (std::floor(d) != d || d < 1.0) { is_array = false; break; }
                intKeys.insert(static_cast<lua_Integer>(d));
            }
            else {
                is_array = false;
                break;
            }
        }
        if (is_array) {
            if (intKeys.size() != keys.size() || *intKeys.rbegin() != static_cast<lua_Integer>(keys.size())) {
                is_array = false;
            }
        }

        // 值转换 lambda
        auto convert_value = [&](const sol::object& val) -> nlohmann::json {
            if (val.is<sol::table>())
                return table_to_json(lua, val.as<sol::table>(), empty_as_array);
            if (val == sol::nil || val.is<sol::nil_t>())
                return nullptr;
            if (val.is<bool>())
                return val.as<bool>();
            if (val.is<lua_Integer>())
                return val.as<lua_Integer>();
            if (val.is<int>())
                return val.as<int>();
            if (val.is<double>()) {
                double d = val.as<double>();
                if (std::isnan(d) || std::isinf(d))
                    return nullptr;
                return d;
            }
            if (val.is<std::string>())
                return val.as<std::string>();
            return nullptr;
            };

        if (is_array) {
            nlohmann::json j = nlohmann::json::array();
            for (size_t i = 1; i <= keys.size(); ++i) {
                sol::object val = t[i];
                j.push_back(convert_value(val));
            }
            return j;
        }
        else {
            nlohmann::json j = nlohmann::json::object();
            for (size_t i = 0; i < keys.size(); ++i) {
                std::string key_str;
                if (keys[i].is<std::string>()) {
                    key_str = keys[i].as<std::string>();
                }
                else if (keys[i].is<lua_Integer>()) {
                    key_str = std::to_string(keys[i].as<lua_Integer>());
                }
                else if (keys[i].is<double>()) {
                    key_str = std::to_string(keys[i].as<double>());
                }
                else if (keys[i].is<int>()) {
                    key_str = std::to_string(keys[i].as<int>());
                }
                else {
                    key_str = "[unserializable key]";
                }
                j[key_str] = convert_value(values[i]);
            }
            return j;
        }
    }

    //注册事件基类
    inline void register_event(sol::state& lua)
    {
        // ========== 1. 注册抽象基类 event ==========
        lua.new_usertype<event>
            (
                "event",
                sol::no_constructor,
                "target_module", &event::target_module,
                "category", &event::category,
                "tag", &event::tag
            );
    }

    //注册配置事件
    inline void register_config_event(sol::state& lua)
    {
        lua.new_usertype<config_event>
            ("config_event",
                sol::base_classes, sol::bases<event>(),
                sol::call_constructor, sol::constructors<>(),
                "config", sol::property
                (
                    [&lua](config_event& self) -> sol::table {
                        return engine::json_to_table(lua, self.config);
                    },
                    [&lua](config_event& self, sol::table t)
                    {
                        self.config = engine::table_to_json(lua, t);
                    }
                )
            );
    }

    //注册实体事件
    inline void register_tracked_event(sol::state& lua)
    {
        lua.new_usertype<tracked_event>("tracked_event",
            sol::base_classes, sol::bases<config_event, event>(), // 多继承链
            sol::call_constructor, sol::constructors<>(), // 允许无参构造
            "sender", &tracked_event::sender,
            "ID", &tracked_event::ID
        );
    }

}
