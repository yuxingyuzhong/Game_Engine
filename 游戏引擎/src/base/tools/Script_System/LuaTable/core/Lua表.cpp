#include "src/base/tools/Script_System/LuaTable/局部命名空间使用.h"

// 从 lua_State 创建一个新的空表，并存入注册表
LuaTable::LuaTable(lua_State* L) : L_(L), ref_(-1)
{
    if (L_) {
        lua_newtable(L_);                     // 压入空表
        ref_ = luaL_ref(L_, LUA_REGISTRYINDEX); // 弹出表并保存引用
    }
}

// 从栈上指定索引捕获一个已有的 table
LuaTable::LuaTable(lua_State* L, int index) : L_(L), ref_(-1)
{
    if (L_) {
        // 仅当索引处的值是 table 时才保存，否则保持无效
        if (lua_type(L_, index) == LUA_TTABLE) {
            lua_pushvalue(L_, index);          // 复制目标到栈顶
            ref_ = luaL_ref(L_, LUA_REGISTRYINDEX);
        }
        else {
            L_ = nullptr;                      // 类型不符，标记为无效
        }
    }
}

// --------------------- 析构 & 资源释放 ---------------------

LuaTable::~LuaTable()
{
    release();
}

void LuaTable::release()
{
    if (L_ && ref_ != -1) {
        luaL_unref(L_, LUA_REGISTRYINDEX, ref_); // 解除注册表引用
    }
    L_ = nullptr;
    ref_ = -1;
}

// --------------------- 移动语义 ---------------------

LuaTable::LuaTable(LuaTable&& other) noexcept
    : L_(other.L_), ref_(other.ref_)
{
    other.L_ = nullptr;
    other.ref_ = -1;
}

LuaTable& LuaTable::operator=(LuaTable&& other) noexcept
{
    if (this != &other) {
        release();               // 释放当前资源
        L_ = other.L_;
        ref_ = other.ref_;
        other.L_ = nullptr;
        other.ref_ = -1;
    }
    return *this;
}

// --------------------- 字段访问接口 ---------------------

bool LuaTable::get_bool(const std::string& key, bool default_value)
{
    if (!is_valid()) return default_value;

    lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_);   // 推入 table
    lua_getfield(L_, -1, key.c_str());          // 推入 table[key]

    bool result = default_value;
    if (lua_isboolean(L_, -1)) {
        result = lua_toboolean(L_, -1);
    }

    lua_pop(L_, 2);                             // 弹出字段与 table
    return result;
}

double LuaTable::get_number(const std::string& key, double default_value) const
{
    if (!is_valid()) return default_value;

    lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_);
    lua_getfield(L_, -1, key.c_str());

    double result = default_value;
    if (lua_isnumber(L_, -1)) {
        result = lua_tonumber(L_, -1);
    }

    lua_pop(L_, 2);
    return result;
}

std::string LuaTable::get_string(const std::string& key, const std::string& default_value) const
{
    if (!is_valid()) return default_value;

    lua_rawgeti(L_, LUA_REGISTRYINDEX, ref_);
    lua_getfield(L_, -1, key.c_str());

    std::string result = default_value;
    if (lua_isstring(L_, -1)) {
        result = lua_tostring(L_, -1);
    }

    lua_pop(L_, 2);
    return result;
}

// --------------------- 状态检查 ---------------------

bool LuaTable::is_valid() const
{
    return L_ != nullptr && ref_ != -1;
}
