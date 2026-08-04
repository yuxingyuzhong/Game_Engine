// 函数功能：哥布林实体初始化脚本
// 由实体管理器在构建哥布林实体时调用
// 用途1：向通用属性槽(pros)写入初始属性
// 用途2：记录行为决策脚本加载路径
function Goblin_Initialize(entity)

    // --- 用途1：通用属性槽初始化 ---

    // 通用属性槽引用
    local pros = entity.pros

    // 最大生命值：哥布林基础生命
    pros.max_hp = 100.0

    // 当前生命值：初始与最大生命值相等
    pros.now_hp = 100.0

    // 基础攻击力：哥布林普通攻击伤害
    pros.attack_power = 12.0

    // 基础防御力：减免所受伤害
    pros.defense = 3.0

    // 移动速度：单位/秒，哥布林体型小速度中等
    pros.move_speed = 4.5

    // 攻击范围：近战攻击距离
    pros.attack_range = 1.5

    // 攻击间隔：两次攻击之间的冷却时间(秒)
    pros.attack_cooldown = 1.2

    // 索敌范围：发现敌人的视野距离
    pros.sight_range = 10.0

    // 初始状态：空表表示无异常状态
    // 后续可通过事件添加 giddy / frozen 等状态
    pros.state_giddy = 0.0
    pros.state_frozen = 0.0

    // --- 用途2：记录行为决策脚本加载路径 ---

    // 行为决策脚本路径：Dynamic_Entity据此加载行为决策脚本
    entity.behavior_script_path = "scripts/behavior/哥布林 (Goblin)_Behavior.lua"

end