// 函数功能：哥布林行为决策主函数
// 由 Dynamic_Entity 每帧调用，接收注入的数据引用，完成行动指令构建与执行
// 参数说明：
//   pros           - 通用属性槽引用 (table)
//   minion_set     - 从属标签槽引用 (table)
//   transfer_buffer - 从属转移缓冲引用 (table)
//   event_set      - 事件集合引用 (table)
//   effect_script_sign - 效应添加接口 (function)
//   wrap_script_sign    - 技能参数打包脚本添加接口 (function)
//   wrap_script_unload  - 技能参数打包脚本移除接口 (function)
//   wrap_script_call    - 技能参数打包脚本执行接口 (function)
//   event_send     - 事件发送入口 (function)
function Goblin_Behavior(pros, minion_set, transfer_buffer, event_set,
                         effect_script_sign, wrap_script_sign, wrap_script_unload, wrap_script_call,
                         event_send)

    // --- 局部变量声明 ---

    // 待执行的行动指令表，由阶段一填充或从 Command 事件读取
    local action_commands = nil

    // 标记是否从事件中读取到了外部指令
    local external_command = false

    // ========================================================================
    // 阶段一：行动指令构建阶段
    // ========================================================================

    // 按“行动指令构建方式”要求，扫描事件集合中是否存在定向命令事件
    // 若存在，则尝试从中读取 Action_Commands 字段，并跳过自主构建

    // 遍历事件集合
    for _, evt in ipairs(event_set) do
        // 检查事件是否为 Entity/Command 类型
        if evt.category == "Entity" and evt.tag == "Command" then
            // 尝试读取外部指令
            local cmd = evt.config and evt.config.Action_Commands
            if cmd ~= nil then
                action_commands = cmd
                external_command = true
            end
            // 无论是否读取成功，都略过阶段一
            break
        end
    end

    // 若无外部指令，则进入自主决策构建行动指令
    if not external_command then

        // 初始化行动指令表
        action_commands = {}

        // --- 自主决策：基础AI逻辑 ---

        // 哥布林AI核心：巡逻-索敌-追击-攻击
        // 决策依据通用属性槽和事件集合中的信息

        // 获取当前生命值，用于判断是否处于战斗状态
        local hp = pros.now_hp
        // 获取最大生命值，用于评估危险程度
        local max_hp = pros.max_hp

        // 索敌逻辑：查找事件集合中是否有“发现敌人”事件
        // 或根据历史状态（此处简化，仅在事件中查找）
        local has_enemy = false
        local enemy_position = nil
        for _, evt in ipairs(event_set) do
            // 假设外部通过 Request 或 config 事件传入感知信息
            if evt.tag == "EnemyDetected" then
                has_enemy = true
                enemy_position = evt.config and evt.config.position
                break
            end
        end

        // 若发现敌人，则构建追击/攻击指令
        if has_enemy and enemy_position then
            // 计算自身位置（此处假设可通过 pros 或其他方式获取，暂用占位）
            // 实际实现时需从引擎获取位置信息，此处为示例性注释
            local my_pos = pros.position  // 假设注入时有位置字段
            if my_pos then
                // 计算与敌人的距离
                local dx = enemy_position.x - my_pos.x
                local dy = enemy_position.y - my_pos.y
                local dist = math.sqrt(dx*dx + dy*dy)

                // 若在攻击范围内，构建攻击指令
                if dist <= pros.attack_range then
                    // 攻击指令：技能名称为“Goblin_Melee”
                    action_commands[#action_commands + 1] = {
                        command = "UseSkill",
                        skill_name = "Goblin_Melee",
                        target = enemy_position
                    }
                else
                    // 否则移动向敌人
                    action_commands[#action_commands + 1] = {
                        command = "MoveTo",
                        target = enemy_position
                    }
                end
            else
                // 无位置信息时原地待命
                action_commands[#action_commands + 1] = {
                    command = "Idle"
                }
            end
        else
            // 无敌人时巡逻：随机移动或待机
            // 此处简化为待机，具体巡逻逻辑可后续完善
            action_commands[#action_commands + 1] = {
                command = "Idle"
            }
        end
    end

    // ========================================================================
    // 阶段二：行动指令执行阶段
    // ========================================================================

    // 遍历所有构建好的行动指令，依次通过技能参数打包组件执行
    // 每个指令会触发对应的技能参数打包脚本，完成参数填充并发送事件
    for _, cmd in ipairs(action_commands) do
        // 根据指令类型调用不同的打包脚本
        if cmd.command == "UseSkill" then
            // 攻击技能：使用对应的技能打包脚本
            // 脚本名称约定为 "wrap_" .. skill_name
            local script_name = "wrap_" .. cmd.skill_name
            // 执行打包脚本，传入指令中的参数
            wrap_script_call(script_name, cmd)
        elseif cmd.command == "MoveTo" then
            // 移动指令：调用移动打包脚本
            wrap_script_call("wrap_MoveTo", cmd)
        elseif cmd.command == "Idle" then
            // 待机指令：调用待机打包脚本（若无则忽略）
            -- 可以不做任何事，或调用空脚本
        end
    end

    // 注意：引擎端会在本函数返回后自动清空事件集合，
    // 无需手动清理 event_set。
end