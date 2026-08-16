# -*- coding: utf-8 -*-
"""一次性修复配置编辑器四个问题（帮助图片路径 / 实体面板属性槽区块 / 联动创建 / 联动删除）"""
import io

path = r'D:\代码存储\代码仓库\游戏引擎\游戏引擎\src\tools\GUI\Config_Editor\core\配置编辑器.cpp'

with io.open(path, encoding='utf-8', newline='') as f:
    content = f.read()

# 记录文件行尾风格（CRLF 或 LF）
uses_crlf = '\r\n' in content

def replace_once(old, new, tag):
    global content
    variants = [old]
    if uses_crlf:
        variants.append(old.replace('\n', '\r\n'))
    for v in variants:
        if v in content:
            content = content.replace(v, new if not uses_crlf else new.replace('\n', '\r\n'), 1)
            print('[OK]', tag)
            return True
    print('[FAIL]', tag, '—— 未找到匹配')
    return False

# ============ A. 帮助图片：移除 E 盘用户路径，只从 assets/UI 加载 ============
old_a = """        //1. 用户指定路径（优先）：E:\\图库\\崩坏星穹铁道\\昔涟\\...
        //   路径含中文，stb 内部 fopen 只认 ANSI，必须转 UTF-16 走 _wfopen + 回调
#ifdef _WIN32
        const char* const 用户图片路径 =
            "E:\\图库\\崩坏星穹铁道\\昔涟\\f49a8541bac6bb14f9331217a3262f9b_4646931331178572486.png";
        const std::wstring 宽路径 = 转宽字符(用户图片路径);
        if (!宽路径.empty())
        {
            像素 = 加载图片宽路径(宽路径, &帮助图片宽, &帮助图片高, &通道数, 4);
            if (像素 != nullptr)
                已用路径 = 用户图片路径;
        }
#endif

        //2. 项目内备份（从用户图片复制而来，保证 exe 自包含、不依赖 E 盘）
        if (像素 == nullptr)
        {
            std::string 路径 = 解析资源路径("assets/UI/cyrene_help.png");
            像素 = stbi_load(路径.c_str(), &帮助图片宽, &帮助图片高, &通道数, 4);
            if (像素 != nullptr)
                已用路径 = 路径;
        }"""
new_a = """        //图片随程序一起分发，固定从 exe 同级 assets/UI/ 目录加载（程序自包含，不依赖外部路径）：
        //  1. 首选 cyrene_help.png（帮助窗口专用图）
        //  2. 缺失时兜底 cyrene_portrait.jpg（旧图）
        //解析资源路径 会先试 CWD、再用 exe 所在目录拼接，保证两种启动方式都能命中
        {
            std::string 路径 = 解析资源路径("assets/UI/cyrene_help.png");
            像素 = stbi_load(路径.c_str(), &帮助图片宽, &帮助图片高, &通道数, 4);
            if (像素 != nullptr)
                已用路径 = 路径;
        }"""
replace_once(old_a, new_a, 'A 帮助图片只从 assets/UI 加载')

# ============ B. 实体面板：移除「属性槽配置（Property_Manager 专用）」区块 ============
old_b = """        // —— 属性槽配置（独立文件，Property_Manager 专用）——
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("属性槽配置（Property_Manager 专用）");
        ImGui::TextDisabled("(独立存储于 config/property/，与实体配置分离)");
        属性槽配置* prop = 仓库.查找属性槽(cfg.type);
        if (prop == nullptr)
        {
            ImGui::TextDisabled("当前实体尚无属性槽配置");
            if (ImGui::Button("创建属性槽配置", ImVec2(180, 0)))
            {
                属性槽配置 新属性槽;
                新属性槽.type = cfg.type;
                新属性槽.initialize_path = "scripts/initialize/" + cfg.type + ".lua";
                std::string 属性槽错误;
                if (仓库.新建属性槽(新属性槽, 属性槽错误))
                    状态消息 = "已创建属性槽配置：" + 新属性槽.type;
                else
                    状态消息 = "属性槽创建失败：" + 属性槽错误;
            }
        }
        else
        {
            ImGui::Text("initialize_path（属性槽初始化脚本）");
            渲染脚本选择器("##initialize_path", prop->initialize_path);
            ImGui::TextDisabled("(非空，相对 assets/ 的 Lua 脚本路径；Property_Manager 用于构建属性槽)");

            if (ImGui::Button("保存属性槽配置", ImVec2(180, 0)))
            {
                std::string 属性槽错误;
                if (仓库.保存属性槽(*prop, 属性槽错误))
                    状态消息 = "已保存属性槽配置：" + prop->config_path;
                else
                    状态消息 = "属性槽保存失败：" + 属性槽错误;
            }
        }

        // —— acls ——"""
new_b = """        // —— acls ——"""
replace_once(old_b, new_b, 'B 实体面板移除属性槽区块')

# ============ C. 新建实体：不再联动创建属性槽配置 ============
old_c = """                std::string error;
                if (仓库.新建实体(cfg, error))
                {
                    属性槽配置 prop;
                    prop.type = name;
                    prop.initialize_path = "scripts/initialize/" + name + ".lua";
                    std::string 属性槽错误;
                    if (仓库.新建属性槽(prop, 属性槽错误))
                        状态消息 = "已创建实体与属性槽配置：" + cfg.type;
                    else
                        状态消息 = "实体已创建，但属性槽配置创建失败：" + 属性槽错误;

                    新条目名输入.clear();"""
new_c = """                std::string error;
                if (仓库.新建实体(cfg, error))
                {
                    状态消息 = "已创建实体配置：" + cfg.type;

                    新条目名输入.clear();"""
replace_once(old_c, new_c, 'C 新建实体不再联动创建属性槽')

# ============ D. 删除实体：不再联动删除属性槽 ============
old_d = """                if (仓库.删除实体(cfg.type, error))
                {
                    仓库.删除属性槽(cfg.type, error);   //同时移除属性槽路由（文件保留）
                    状态消息 = "已删除实体（路由条目）：" + cfg.type;
                    选中索引 = -1;
                }"""
new_d = """                if (仓库.删除实体(cfg.type, error))
                {
                    状态消息 = "已删除实体（路由条目）：" + cfg.type;
                    选中索引 = -1;
                }"""
replace_once(old_d, new_d, 'D 删除实体不再联动删除属性槽')

with io.open(path, 'w', encoding='utf-8', newline='') as f:
    f.write(content)
print('=== 脚本完成 ===')
