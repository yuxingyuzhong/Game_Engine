#include "src/tools/GUI/Config_Editor/配置编辑器.h"

//标准库：宽字符文件操作（_wfopen）与文件回调（fread/fseek/feof）
#include <cstdio>
#include <cwchar>

//stb_image：单头文件图片解码库（本编译单元唯一实现，其他文件只 include 声明）
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb/stb_image.h"

//引擎命名空间
namespace engine
{
    //========================================================================
    // 内部工具
    //========================================================================
    namespace
    {
        //解析资源文件的绝对路径：
        //  1. 优先尝试当前工作目录（CWD = exe 所在目录时的常见情况）
        //  2. 失败则用 exe 所在目录拼接（防止 CWD 被外部改变）
        std::string 解析资源路径(const char* 相对路径)
        {
            {
                std::ifstream 探测(相对路径);
                if (探测.good())
                {
                    探测.close();
                    return 相对路径;
                }
            }
#ifdef _WIN32
            char 缓冲[MAX_PATH] = {};
            if (GetModuleFileNameA(nullptr, 缓冲, MAX_PATH) > 0)
            {
                std::filesystem::path exe路径(缓冲);
                std::filesystem::path 候选 = exe路径.parent_path() / 相对路径;
                std::ifstream 探测(候选);
                if (探测.good())
                {
                    探测.close();
                    return 候选.string();
                }
            }
#endif
            return 相对路径;   //都失败就按原路径交给 stb 再试一次
        }

        //std::string 输入框（原理同 imgui_stdlib：直接绑定 std::string 内部缓冲）
        //返回是否发生修改
        bool 输入文本(const char* label, std::string& value)
        {
            //确保缓冲区可写且以 \0 结尾
            if (value.capacity() < value.size() + 1)
                value.reserve(value.size() + 16);
            char* buffer = value.data();   // C++17 起 data() 可写
            buffer[value.size()] = '\0';

            bool changed = ImGui::InputText(label, buffer, value.capacity(), 0);
            if (changed)
                value.resize(std::strlen(buffer));
            return changed;
        }

        //带提示文本的输入框（空值时显示灰色提示）
        bool 输入文本提示(const char* label, std::string& value, const char* hint)
        {
            if (value.capacity() < value.size() + 1)
                value.reserve(value.size() + 16);
            char* buffer = value.data();
            buffer[value.size()] = '\0';

            bool changed = ImGui::InputTextWithHint(label, hint, buffer, value.capacity(), 0);
            if (changed)
                value.resize(std::strlen(buffer));
            return changed;
        }

        //生成字段默认值（按字段类型，用于通用配置字段缺失时补默认）
        nlohmann::json 生成默认值(const 配置字段定义& f)
        {
            switch (f.类型)
            {
            case 配置字段类型::文本:
            case 配置字段类型::脚本路径:
                return "";
            case 配置字段类型::文本列表:
            case 配置字段类型::事件对列表:
                return nlohmann::json::array();
            case 配置字段类型::整数:
                return 0;
            case 配置字段类型::浮点数:
                return 0.0;
            case 配置字段类型::布尔:
                return false;
            }
            return "";
        }
    }

    //========================================================================
    // 配置编辑器
    //========================================================================

    //构造函数
    配置编辑器::配置编辑器()
    {
    }

    //加载帮助窗口右侧的昔涟图片（stb_image 解码 + OpenGL 纹理）
    //只在首次需要时加载一次；失败则保持纹理为 0，渲染层回退为字符画
    bool 配置编辑器::加载帮助图片()
    {
        if (帮助图片已尝试)
            return 帮助图片纹理 != 0;
        帮助图片已尝试 = true;

        //请求 RGBA 通道（简化纹理格式，避免行对齐问题）
        //注意：不要翻转！ImGui 的纹理约定是 uv(0,0)=纹理左上角=数据首行，
        //stb_image 默认首行就是图像顶部，直接上传即正立；翻转反而会上下颠倒
        stbi_set_flip_vertically_on_load(false);
        int 通道数 = 0;
        unsigned char* 像素 = nullptr;
        std::string 已用路径;

        //图片随程序一起分发，固定从 exe 同级 assets/UI/ 目录加载（程序自包含，不依赖外部路径）：
        //  1. 首选 cyrene_help.png（帮助窗口专用图）
        //  2. 缺失时兜底 cyrene_portrait.jpg（旧图）
        //解析资源路径 会先试 CWD、再用 exe 所在目录拼接，保证两种启动方式都能命中
        {
            std::string 路径 = 解析资源路径("assets/UI/cyrene_help.png");
            像素 = stbi_load(路径.c_str(), &帮助图片宽, &帮助图片高, &通道数, 4);
            if (像素 != nullptr)
                已用路径 = 路径;
        }

        //2. 兜底：旧图
        if (像素 == nullptr)
        {
            std::string 路径 = 解析资源路径("assets/UI/cyrene_portrait.jpg");
            像素 = stbi_load(路径.c_str(), &帮助图片宽, &帮助图片高, &通道数, 4);
            if (像素 != nullptr)
                已用路径 = 路径;
        }

        if (像素 == nullptr || 帮助图片宽 <= 0 || 帮助图片高 <= 0)
        {
            std::printf("[ConfigEditor] 警告：无法加载帮助图片（%s）\n",
                stbi_failure_reason() ? stbi_failure_reason() : "未知错误");
            帮助图片宽 = 帮助图片高 = 0;
            return false;
        }

        //创建 OpenGL 纹理
        glGenTextures(1, &帮助图片纹理);
        glBindTexture(GL_TEXTURE_2D, 帮助图片纹理);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 帮助图片宽, 帮助图片高,
            0, GL_RGBA, GL_UNSIGNED_BYTE, 像素);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(像素);

        std::printf("[ConfigEditor] 已加载帮助图片：%s（%dx%d）\n",
            已用路径.c_str(), 帮助图片宽, 帮助图片高);
        return true;
    }

    //加载配置
    void 配置编辑器::加载()
    {
        //加载实体配置
        if (仓库.加载())
        {
            加载消息 = "已加载 " + std::to_string(仓库.获取全部().size()) +
                " 个实体配置、" + std::to_string(仓库.获取属性槽全部().size()) + " 个属性槽配置";
            状态消息.clear();
        }
        else
        {
            加载消息 = "配置加载失败：无法读取路由表 assets/config/route/entity.json";
            状态消息 = 加载消息;
        }

        //刷新脚本候选列表
        脚本列表 = 仓库.获取脚本列表();
        //重置选中项
        选中索引 = -1;

        //自检日志：数据层加载统计 + 帮助窗口初始状态（便于排查启动类问题）
        std::printf("[ConfigEditor] 加载完成：实体 %zu、属性槽 %zu、格式 %zu、通用配置 %zu；帮助窗口初始=%s\n",
            仓库.获取全部().size(),
            仓库.获取属性槽全部().size(),
            仓库.获取格式全部().size(),
            仓库.获取通用配置全部().size(),
            显示帮助窗口 ? "开" : "关");
    }

    //每帧渲染
    void 配置编辑器::渲染()
    {
        //主窗口（占满视口）
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("实体配置编辑器", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar);

        //菜单栏
        渲染菜单栏();

        //左右分栏（左栏宽度自适应：窗口宽度的 27%，限制在 300~520 之间，让列表和输入框更宽）
        float 状态栏高度 = 30.0f;
        float 分栏高度 = ImGui::GetContentRegionAvail().y - 状态栏高度;
        float 左栏宽度 = ImGui::GetIO().DisplaySize.x * 0.27f;
        if (左栏宽度 < 300.0f) 左栏宽度 = 300.0f;
        if (左栏宽度 > 520.0f) 左栏宽度 = 520.0f;
        ImGui::BeginChild("##左栏", ImVec2(左栏宽度, 分栏高度), true);
        //新建配置区（模块选择 + 条目名 + 创建按钮）
        渲染新建配置区();
        ImGui::Separator();
        //按 当前模块 显示配置列表
        渲染模块列表();
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("##右栏", ImVec2(0, 分栏高度), true);
        //按 当前模块 渲染右侧面板
        渲染模块面板();
        ImGui::EndChild();

        //状态栏
        渲染状态栏();

        ImGui::End();

        //F1 快捷切换帮助窗口
        if (ImGui::IsKeyPressed(ImGuiKey_F1))
            显示帮助窗口 = !显示帮助窗口;

        //帮助窗口（独立小窗：可拖动/缩放，内容可滚动，右侧有笑脸）
        渲染帮助窗口();

        //配置格式管理窗口（工具菜单打开）
        渲染格式管理窗口();
    }

    //渲染菜单栏
    void 配置编辑器::渲染菜单栏()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("文件"))
            {
                if (ImGui::MenuItem("重新加载", "F5"))
                {
                    加载();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("保存当前实体"))
                {
                    if (选中索引 >= 0 && 选中索引 < (int)仓库.获取全部().size())
                    {
                        std::string error;
                        实体配置& cfg = 仓库.获取全部()[选中索引];
                        if (仓库.保存实体(cfg, error))
                            状态消息 = "已保存：" + cfg.config_path;
                        else
                            状态消息 = "保存失败：" + error;
                    }
                    else
                        状态消息 = "请先选择一个实体";
                }
                if (ImGui::MenuItem("保存全部"))
                {
                    int 成功数 = 0;
                    for (auto& cfg : 仓库.获取全部())
                    {
                        std::string error;
                        if (仓库.保存实体(cfg, error))
                            成功数++;
                    }
                    状态消息 = "保存全部完成：" + std::to_string(成功数) +
                        "/" + std::to_string(仓库.获取全部().size()) + " 个实体";
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("帮助"))
            {
                if (ImGui::MenuItem("使用说明", "F1"))
                    显示帮助窗口 = true;
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("工具"))
            {
                if (ImGui::MenuItem("配置格式管理", nullptr, &显示格式管理))
                {
                    //打开时同步当前选中格式到编辑副本
                    if (显示格式管理)
                    {
                        if (仓库.获取格式全部().empty())
                            仓库.加载格式();
                        if (!仓库.获取格式全部().empty())
                        {
                            格式编辑 = 仓库.获取格式全部().front();
                            格式编辑有效 = true;
                        }
                        else
                            格式编辑有效 = false;
                    }
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }
    }

    //渲染帮助窗口（独立小窗：正文自动换行可滚动，右侧显示昔涟图片）
    void 配置编辑器::渲染帮助窗口()
    {
        if (!显示帮助窗口)
            return;

        //首次出现时居中显示，尺寸按屏幕比例放大（小窗太小的老问题：至少 860×560）
        const ImVec2 屏幕 = ImGui::GetIO().DisplaySize;
        float 帮助宽 = 屏幕.x * 0.55f;
        float 帮助高 = 屏幕.y * 0.72f;
        if (帮助宽 < 860.0f) 帮助宽 = 860.0f;
        if (帮助高 < 560.0f) 帮助高 = 560.0f;
        if (帮助宽 > 1200.0f) 帮助宽 = 1200.0f;
        if (帮助高 > 860.0f) 帮助高 = 860.0f;
        ImGui::SetNextWindowPos(
            ImVec2(屏幕.x * 0.5f, 屏幕.y * 0.5f),
            ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(帮助宽, 帮助高), ImGuiCond_Appearing);

        //标题栏右侧自带 × 关闭按钮，点击后 显示帮助窗口 自动置 false
        //—— 背景不透明处理（粉色系，与主界面统一；完全不透明避免透出主界面文字）——
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.55f, 0.32f, 0.58f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.72f, 0.48f, 0.80f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.72f, 0.38f, 0.68f, 1.00f));
        if (ImGui::Begin("使用说明", &显示帮助窗口, ImGuiWindowFlags_NoCollapse))
        {
            //左右分栏：左侧帮助正文（自动换行 + 滚动），右侧昔涟图片
            float 可用宽度 = ImGui::GetContentRegionAvail().x;
            float 左栏宽度 = 可用宽度 * 0.58f;

            // —— 左侧：帮助正文（自动换行；内容超出自动滚动）——
            ImGui::BeginChild("##帮助正文", ImVec2(左栏宽度, 0), true);
            {
                ImGui::PushTextWrapPos(0.0f);   //0.0f = 换行到当前区域右缘

                ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.94f, 1.0f), "《实体配置字段说明》（config/entities/）");
                ImGui::TextWrapped("• type：实体类型标识（必填，非空，保存后配置文件会自动迁移）");
                ImGui::TextWrapped("• decision_load_path：决策树行为脚本路径（必填，相对 assets/ 目录，Entity_Manager 使用；旧配置缺少该字段时需补填）");
                ImGui::TextWrapped("• acls：从属权限列表（必填，非空，引擎加载时逐项校验）");
                ImGui::TextWrapped("• needed_events：订阅事件列表（必填，非空，每项含 事件源/事件名）");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.94f, 1.0f), "《属性槽配置字段说明》（config/property/）");
                ImGui::TextWrapped("• type：实体类型标识（必填，与实体配置 type 一致）");
                ImGui::TextWrapped("• initialize_path：属性槽初始化 Lua 脚本路径（必填，相对 assets/ 目录，Property_Manager 使用）");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.94f, 1.0f), "《常用操作》");
                ImGui::TextWrapped("• 左侧列表点选实体，右侧即可编辑属性，修改后记得保存");
                ImGui::TextWrapped("• 「创建配置」可新建实体，自动生成默认 acls 与 needed_events");
                ImGui::TextWrapped("• 修改 type 后保存，配置文件会自动迁移到新路径");
                ImGui::TextWrapped("• 删除实体仅移除路由条目，实体配置文件本身会保留");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.94f, 1.0f), "《校验规则》");
                ImGui::TextWrapped("• 引擎 Entity_Manager::config_field_parse 要求 acls 与 needed_events 均不能为空，否则引擎拒绝加载");
                ImGui::TextWrapped("• 右侧「校验结果」面板可实时查看错误与警告，全部通过后保存更稳妥");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.94f, 1.0f), "《快捷键》");
                ImGui::TextWrapped("• F1：打开 / 关闭本帮助窗口");
                ImGui::TextWrapped("• F5：重新加载全部配置（文件菜单中也有）");

                ImGui::PopTextWrapPos();
            }
            ImGui::EndChild();

            ImGui::SameLine();

            // —— 右侧：昔涟的图片（懒加载纹理；加载失败时回退为字符画）——
            ImGui::BeginChild("##帮助笑脸", ImVec2(0, 0));
            {
                if (!加载帮助图片())
                {
                    //回退：粉色字符画（图片缺失时仍保留笑脸）
                    const char* 笑脸[] =
                    {
                        "   ♪ ／￣＼　／￣＼ ♪",
                        "    （ ＾ω＾ ）",
                        "     ＼＿／＼＿／",
                        "",
                        "「愿世界，如你我所愿♪」",
                    };
                    const int 行数 = (int)(sizeof(笑脸) / sizeof(笑脸[0]));
                    const float 行高 = ImGui::GetTextLineHeightWithSpacing();
                    float 可用高 = ImGui::GetContentRegionAvail().y;
                    float 内容高 = 行数 * 行高;
                    if (可用高 > 内容高)
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (可用高 - 内容高) * 0.5f);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.70f, 0.88f, 1.0f));
                    for (int i = 0; i < 行数; ++i)
                    {
                        float 文本宽 = ImGui::CalcTextSize(笑脸[i]).x;
                        float 区域宽 = ImGui::GetContentRegionAvail().x;
                        if (文本宽 < 区域宽)
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (区域宽 - 文本宽) * 0.5f);
                        ImGui::TextUnformatted(笑脸[i]);
                    }
                    ImGui::PopStyleColor();
                }
                else
                {
                    //图片：按区域等比缩放、整体垂直居中
                    float 区域宽 = ImGui::GetContentRegionAvail().x - 10.0f;
                    float 区域高 = ImGui::GetContentRegionAvail().y - 10.0f;
                    float 图片比例 = (float)帮助图片宽 / (float)帮助图片高;
                    float 显示宽 = 区域宽;
                    float 显示高 = 显示宽 / 图片比例;
                    if (显示高 > 区域高)
                    {
                        显示高 = 区域高;
                        显示宽 = 显示高 * 图片比例;
                    }

                    const float 行高 = ImGui::GetTextLineHeightWithSpacing();
                    const float 提示高 = 行高 + 8.0f;
                    float 内容总高 = 显示高 + 提示高;
                    if (区域高 > 内容总高)
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (区域高 - 内容总高) * 0.5f);

                    //水平居中
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (区域宽 - 显示宽) * 0.5f);
                    ImGui::Image((ImTextureID)(intptr_t)帮助图片纹理, ImVec2(显示宽, 显示高));

                    //图片下方的小字（居中）
                    const char* 寄语 = "「愿世界，如你我所愿♪」";
                    float 文本宽 = ImGui::CalcTextSize(寄语).x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (区域宽 - 文本宽) * 0.5f);
                    ImGui::TextColored(ImVec4(1.0f, 0.80f, 0.94f, 1.0f), "%s", 寄语);
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
        //恢复默认背景样式
        ImGui::PopStyleColor(3);
    }

    //渲染左侧新建配置区（模块选择 + 条目名 + 创建按钮）
    void 配置编辑器::渲染新建配置区()
    {
        // —— 目标模块选择 ——
        ImGui::Text("新建配置");
        //模块下拉：内置两种 + 自定义模块
        {
            //构造模块候选列表
            std::vector<std::string> 候选;
            std::vector<std::string> 标签;
            for (const auto& fmt : 仓库.获取格式全部())
            {
                候选.push_back(fmt.模块名);
                标签.push_back(fmt.模块名 + (fmt.内置 ? "（内置）" : ""));
            }
            if (候选.empty())
            {
                候选.push_back("Entity_Manager");
                标签.push_back("Entity_Manager（内置）");
            }

            //确保 新建模块 是有效候选
            if (std::find(候选.begin(), 候选.end(), 新建模块) == 候选.end())
                新建模块 = 候选.front();

            //当前选中项在候选中的索引
            int 当前索引 = 0;
            for (int i = 0; i < (int)候选.size(); ++i)
                if (候选[i] == 新建模块)
                {
                    当前索引 = i;
                    break;
                }

            if (ImGui::BeginCombo("##新建模块", 标签[当前索引].c_str(), 0))
            {
                for (int i = 0; i < (int)候选.size(); ++i)
                {
                    bool selected = (i == 当前索引);
                    if (ImGui::Selectable(标签[i].c_str(), selected))
                    {
                        新建模块 = 候选[i];
                        //切换模块后清空名称输入，避免误带到新格式
                        新条目名输入.clear();
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::TextDisabled("(选择目标模块后按格式创建配置)");

        // —— 条目名输入 ——
        输入文本提示("##新条目名", 新条目名输入, "条目名（如 Goblin / MyItem）");

        // —— 创建按钮 ——
        if (ImGui::Button("创建配置", ImVec2(-1, 0)))
        {
            //去掉首尾空白
            std::string name = 新条目名输入;
            while (!name.empty() && name.front() == ' ') name.erase(name.begin());
            while (!name.empty() && name.back() == ' ') name.pop_back();

            if (name.empty())
            {
                状态消息 = "条目名不能为空";
            }
            else if (新建模块 == "Entity_Manager")
            {
                //实体格式：仅创建实体配置（属性槽配置请在 Property_Manager 模块下创建）
                实体配置 cfg;
                cfg.type = name;
                cfg.decision_load_path = "scripts/behavior/" + name + "_Behavior.lua";
                cfg.acls.push_back(name);
                cfg.needed_events.emplace_back("Entity", "Request");

                std::string error;
                if (仓库.新建实体(cfg, error))
                {
                    状态消息 = "已创建实体配置：" + cfg.type;

                    新条目名输入.clear();
                    脚本列表 = 仓库.获取脚本列表();
                    当前模块 = "Entity_Manager";
                    选中索引 = (int)仓库.获取全部().size() - 1;
                }
                else
                    状态消息 = "创建失败：" + error;
            }
            else if (新建模块 == "Property_Manager")
            {
                //属性槽格式：仅创建属性槽配置
                属性槽配置 prop;
                prop.type = name;
                prop.initialize_path = "scripts/initialize/" + name + ".lua";
                std::string error;
                if (仓库.新建属性槽(prop, error))
                {
                    状态消息 = "已创建属性槽配置：" + prop.type;
                    新条目名输入.clear();
                    脚本列表 = 仓库.获取脚本列表();
                    当前模块 = "Property_Manager";
                    选中索引 = (int)仓库.获取属性槽全部().size() - 1;
                }
                else
                    状态消息 = "创建失败：" + error;
            }
            else
            {
                //自定义模块：按格式定义创建通用配置
                配置格式* fmt = 仓库.查找格式(新建模块);
                if (fmt == nullptr)
                {
                    状态消息 = "模块格式不存在：" + 新建模块;
                }
                else
                {
                    通用配置 cfg;
                    cfg.模块名 = 新建模块;
                    cfg.条目名 = name;
                    cfg.字段值 = 仓库.生成默认字段值(*fmt);
                    //若格式定义含 type/name 字段，将条目名填入
                    if (cfg.字段值.contains("type") && cfg.字段值["type"].is_string())
                        cfg.字段值["type"] = name;
                    else if (cfg.字段值.contains("name") && cfg.字段值["name"].is_string())
                        cfg.字段值["name"] = name;

                    std::string error;
                    if (仓库.新建通用配置(cfg, error))
                    {
                        状态消息 = "已创建配置：" + 新建模块 + "/" + cfg.条目名;
                        新条目名输入.clear();
                        当前模块 = 新建模块;
                        选中通用配置索引 = (int)仓库.获取通用配置全部().size() - 1;
                    }
                    else
                        状态消息 = "创建失败：" + error;
                }
            }
        }
    }

    //渲染左侧模块列表（按 当前模块 显示对应配置）
    void 配置编辑器::渲染模块列表()
    {
        // —— 当前模块下拉（浏览切换）——
        {
            std::vector<std::string> 候选;
            std::vector<std::string> 标签;
            for (const auto& fmt : 仓库.获取格式全部())
            {
                候选.push_back(fmt.模块名);
                标签.push_back(fmt.模块名 + (fmt.内置 ? "（内置）" : ""));
            }
            if (候选.empty())
            {
                候选.push_back("Entity_Manager");
                标签.push_back("Entity_Manager（内置）");
            }
            if (std::find(候选.begin(), 候选.end(), 当前模块) == 候选.end())
                当前模块 = 候选.front();

            int 当前索引 = 0;
            for (int i = 0; i < (int)候选.size(); ++i)
                if (候选[i] == 当前模块)
                {
                    当前索引 = i;
                    break;
                }

            ImGui::Text("当前模块：");
            if (ImGui::BeginCombo("##当前模块", 标签[当前索引].c_str(), 0))
            {
                for (int i = 0; i < (int)候选.size(); ++i)
                {
                    bool selected = (i == 当前索引);
                    if (ImGui::Selectable(标签[i].c_str(), selected))
                    {
                        当前模块 = 候选[i];
                        //切换模块时重置选中状态
                        选中索引 = -1;
                        选中通用配置索引 = -1;
                        待确认删除类型.clear();
                        待确认删除条目.clear();
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::Separator();

        //搜索过滤
        输入文本提示("##搜索", 搜索文本, "搜索…");
        ImGui::Separator();

        if (当前模块 == "Entity_Manager")
        {
            // —— 实体配置列表 ——
            const auto& 实体集合 = 仓库.获取全部();
            ImGui::BeginChild("##实体列表滚动");
            for (int i = 0; i < (int)实体集合.size(); ++i)
            {
                const auto& cfg = 实体集合[i];
                if (!搜索文本.empty())
                {
                    if (cfg.type.find(搜索文本) == std::string::npos &&
                        cfg.config_path.find(搜索文本) == std::string::npos)
                        continue;
                }
                bool selected = (i == 选中索引);
                if (ImGui::Selectable(cfg.type.c_str(), selected))
                    选中索引 = i;
            }
            ImGui::EndChild();
        }
        else if (当前模块 == "Property_Manager")
        {
            // —— 属性槽配置列表 ——
            const auto& 属性槽集合 = 仓库.获取属性槽全部();
            ImGui::BeginChild("##属性槽列表滚动");
            for (int i = 0; i < (int)属性槽集合.size(); ++i)
            {
                const auto& cfg = 属性槽集合[i];
                if (!搜索文本.empty())
                {
                    if (cfg.type.find(搜索文本) == std::string::npos &&
                        cfg.config_path.find(搜索文本) == std::string::npos)
                        continue;
                }
                bool selected = (i == 选中索引);
                if (ImGui::Selectable(cfg.type.c_str(), selected))
                    选中索引 = i;
            }
            ImGui::EndChild();
        }
        else
        {
            // —— 自定义模块通用配置列表 ——
            const auto& 通用集合 = 仓库.获取通用配置全部();
            ImGui::BeginChild("##通用列表滚动");
            for (int i = 0; i < (int)通用集合.size(); ++i)
            {
                const auto& cfg = 通用集合[i];
                if (cfg.模块名 != 当前模块)
                    continue;
                if (!搜索文本.empty())
                {
                    if (cfg.条目名.find(搜索文本) == std::string::npos &&
                        cfg.config_path.find(搜索文本) == std::string::npos)
                        continue;
                }
                bool selected = (i == 选中通用配置索引);
                if (ImGui::Selectable(cfg.条目名.c_str(), selected))
                    选中通用配置索引 = i;
            }
            ImGui::EndChild();
        }
    }

    //渲染右侧面板（按 当前模块 分发）
    void 配置编辑器::渲染模块面板()
    {
        if (当前模块 == "Entity_Manager")
        {
            if (选中索引 >= 0 && 选中索引 < (int)仓库.获取全部().size())
                渲染属性面板();
            else
                ImGui::TextWrapped("请从左侧列表选择一个实体进行编辑，或点击「创建配置」。");
        }
        else if (当前模块 == "Property_Manager")
        {
            if (选中索引 >= 0 && 选中索引 < (int)仓库.获取属性槽全部().size())
                渲染属性槽面板();
            else
                ImGui::TextWrapped("请从左侧列表选择一个属性槽配置进行编辑，或点击「创建配置」。");
        }
        else
        {
            if (选中通用配置索引 >= 0 &&
                选中通用配置索引 < (int)仓库.获取通用配置全部().size() &&
                仓库.获取通用配置全部()[选中通用配置索引].模块名 == 当前模块)
                渲染通用配置面板();
            else
                ImGui::TextWrapped("请从左侧列表选择一个配置进行编辑，或点击「创建配置」。");
        }
    }

    //渲染右侧属性面板
    void 配置编辑器::渲染属性面板()
    {
        if (选中索引 < 0 || 选中索引 >= (int)仓库.获取全部().size())
            return;
        实体配置& cfg = 仓库.获取全部()[选中索引];

        // —— 头部 ——
        ImGui::Text("实体配置：%s", cfg.type.c_str());
        ImGui::TextDisabled("配置文件：%s", cfg.config_path.c_str());
        ImGui::Separator();

        // —— type ——（全宽输入框）
        ImGui::Text("type（实体类型）");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (输入文本("##type", cfg.type))
            状态消息 = "注意：修改 type 后保存时将自动迁移配置文件";
        ImGui::TextDisabled("(非空)");

        // —— decision_load_path ——
        ImGui::Spacing();
        ImGui::Text("decision_load_path（决策树行为脚本）");
        渲染脚本选择器("##decision_path", cfg.decision_load_path);
        ImGui::TextDisabled("(非空，相对 assets/ 的 Lua 脚本路径；Entity_Manager 用于加载实体行为决策树)");

        // —— acls ——
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("acls（从属权限列表）");
        ImGui::TextDisabled("(非空；master 为当前实体类型，此处填写允许从属的实体类型)");
        渲染权限列表(cfg);

        // —— needed_events ——
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("needed_events（订阅事件列表）");
        ImGui::TextDisabled("(非空；每项 = [分类, 标签]，如 [Entity, Request])");
        渲染事件列表(cfg);

        // —— 操作按钮 ——
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("保存当前实体", ImVec2(160, 0)))
        {
            std::string error;
            if (仓库.保存实体(cfg, error))
                状态消息 = "已保存：" + cfg.config_path;
            else
                状态消息 = "保存失败：" + error;
        }
        ImGui::SameLine();
        //删除按钮（二次确认）
        bool 正在确认 = (待确认删除类型 == cfg.type);
        if (ImGui::Button(正在确认 ? "再次点击确认删除" : "删除实体",
            ImVec2(180, 0)))
        {
            if (!正在确认)
                待确认删除类型 = cfg.type;
            else
            {
                std::string error;
                if (仓库.删除实体(cfg.type, error))
                {
                    状态消息 = "已删除实体（路由条目）：" + cfg.type;
                    选中索引 = -1;
                }
                else
                    状态消息 = "删除失败：" + error;
                待确认删除类型.clear();
            }
        }
        if (正在确认)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("取消"))
                待确认删除类型.clear();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                "注意：仅移除路由条目，实体文件保留");
        }

        // —— 校验结果（可折叠，默认展开但列表限高，不挤占编辑区）——
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::CollapsingHeader("校验结果"))
        {
            渲染校验结果(cfg);
        }
    }

    //渲染校验结果
    void 配置编辑器::渲染校验结果(const 实体配置& cfg)
    {
        std::vector<std::string> errors, warnings;
        bool 通过 = 仓库.校验(cfg, errors, warnings);

        ImGui::Text("校验结果：");
        if (通过 && warnings.empty())
        {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "  ✓ 校验通过，配置可被引擎加载");
        }
        else
        {
            if (!errors.empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                    "  ✗ %d 个错误（将导致引擎拒绝加载）：", (int)errors.size());
                //错误列表限高滚动，避免挤占编辑区空间
                ImGui::BeginChild("##错误列表", ImVec2(0, 110.0f), true);
                for (const auto& e : errors)
                    ImGui::BulletText("%s", e.c_str());
                ImGui::EndChild();
            }
            if (!warnings.empty())
            {
                ImGui::TextColored(ImVec4(0.95f, 0.8f, 0.3f, 1.0f),
                    "  ! %d 个警告（不影响加载，但建议修复）：", (int)warnings.size());
                //警告列表限高滚动
                ImGui::BeginChild("##警告列表", ImVec2(0, 90.0f), true);
                for (const auto& w : warnings)
                    ImGui::BulletText("%s", w.c_str());
                ImGui::EndChild();
            }
        }
    }

    //渲染 acls 编辑列表
    void 配置编辑器::渲染权限列表(实体配置& cfg)
    {
        int 删除索引 = -1;

        for (int i = 0; i < (int)cfg.acls.size(); ++i)
        {
            std::string label = "##acl_" + std::to_string(i);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
            输入文本(label.c_str(), cfg.acls[i]);
            ImGui::SameLine();
            if (ImGui::SmallButton(("删除##delacl_" + std::to_string(i)).c_str()))
                删除索引 = i;
        }

        //移除被删除的条目
        if (删除索引 >= 0)
            cfg.acls.erase(cfg.acls.begin() + 删除索引);

        //添加新条目
        if (ImGui::SmallButton("+ 添加从属类型"))
            cfg.acls.push_back(std::string());
    }

    //渲染 needed_events 编辑列表
    void 配置编辑器::渲染事件列表(实体配置& cfg)
    {
        int 删除索引 = -1;

        for (int i = 0; i < (int)cfg.needed_events.size(); ++i)
        {
            //分类输入（均分剩余宽度，加宽输入框）
            float 宽度 = (ImGui::GetContentRegionAvail().x - 60.0f) * 0.5f;
            ImGui::SetNextItemWidth(宽度);
            输入文本(("##evt_cat_" + std::to_string(i)).c_str(), cfg.needed_events[i].first);
            ImGui::SameLine();
            //标签输入
            ImGui::SetNextItemWidth(宽度);
            输入文本(("##evt_tag_" + std::to_string(i)).c_str(), cfg.needed_events[i].second);
            ImGui::SameLine();
            if (ImGui::SmallButton(("删除##delevt_" + std::to_string(i)).c_str()))
                删除索引 = i;
        }

        if (删除索引 >= 0)
            cfg.needed_events.erase(cfg.needed_events.begin() + 删除索引);

        if (ImGui::SmallButton("+ 添加订阅事件"))
            cfg.needed_events.emplace_back("Entity", "Request");
    }

    //渲染脚本选择器（文本输入 + 脚本下拉，label 用于避免多个选择器 ID 冲突，path 为要编辑的目标路径字段）
    void 配置编辑器::渲染脚本选择器(const char* label, std::string& path)
    {
        //文本输入（label 拼接后缀，避免多个选择器 ID 冲突）
        std::string 输入框标签 = std::string(label) + "_input";
        float 宽度 = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(宽度);
        输入文本(输入框标签.c_str(), path);

        //脚本下拉选择
        std::string 下拉标签 = std::string(label) + "_combo";
        if (ImGui::BeginCombo(下拉标签.c_str(), "从现有脚本选择…", 0))
        {
            for (const auto& script : 脚本列表)
            {
                bool selected = (script == path);
                if (ImGui::Selectable(script.c_str(), selected))
                    path = script;
            }
            ImGui::EndCombo();
        }

        //刷新脚本列表按钮
        ImGui::SameLine();
        if (ImGui::SmallButton("刷新"))
            脚本列表 = 仓库.获取脚本列表();
    }

    //渲染属性槽配置面板（Property_Manager 模块）
    void 配置编辑器::渲染属性槽面板()
    {
        if (选中索引 < 0 || 选中索引 >= (int)仓库.获取属性槽全部().size())
            return;
        属性槽配置& prop = 仓库.获取属性槽全部()[选中索引];

        // —— 头部 ——
        ImGui::Text("属性槽配置：%s", prop.type.c_str());
        ImGui::TextDisabled("配置文件：%s", prop.config_path.c_str());
        ImGui::Separator();

        // —— type ——
        ImGui::Text("type（实体类型）");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (输入文本("##prop_type", prop.type))
            状态消息 = "注意：修改 type 后保存时将自动迁移配置文件";
        ImGui::TextDisabled("(非空)");

        // —— initialize_path ——
        ImGui::Spacing();
        ImGui::Text("initialize_path（属性槽初始化脚本）");
        渲染脚本选择器("##prop_initialize", prop.initialize_path);
        ImGui::TextDisabled("(非空，相对 assets/ 的 Lua 脚本路径；Property_Manager 用于构建属性槽)");

        // —— 操作按钮 ——
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("保存属性槽配置", ImVec2(180, 0)))
        {
            std::string error;
            if (仓库.保存属性槽(prop, error))
                状态消息 = "已保存属性槽配置：" + prop.config_path;
            else
                状态消息 = "保存失败：" + error;
        }
        ImGui::SameLine();
        bool 正在确认 = (待确认删除条目 == std::string("属性槽:") + prop.type);
        if (ImGui::Button(正在确认 ? "再次点击确认删除" : "删除属性槽配置", ImVec2(200, 0)))
        {
            if (!正在确认)
                待确认删除条目 = std::string("属性槽:") + prop.type;
            else
            {
                std::string error;
                if (仓库.删除属性槽(prop.type, error))
                {
                    状态消息 = "已删除属性槽配置（路由条目）：" + prop.type;
                    选中索引 = -1;
                }
                else
                    状态消息 = "删除失败：" + error;
                待确认删除条目.clear();
            }
        }
        if (正在确认)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("取消"))
                待确认删除条目.clear();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                "注意：仅移除路由条目，配置文件保留");
        }

        // —— 校验结果 ——
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::CollapsingHeader("校验结果"))
        {
            std::vector<std::string> errors, warnings;
            bool 通过 = 仓库.校验属性槽(prop, errors, warnings);
            ImGui::Text("校验结果：");
            if (通过 && warnings.empty())
            {
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "  ✓ 校验通过");
            }
            else
            {
                if (!errors.empty())
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                        "  ✗ %d 个错误：", (int)errors.size());
                    ImGui::BeginChild("##属性槽错误列表", ImVec2(0, 90.0f), true);
                    for (const auto& e : errors)
                        ImGui::BulletText("%s", e.c_str());
                    ImGui::EndChild();
                }
                if (!warnings.empty())
                {
                    ImGui::TextColored(ImVec4(0.95f, 0.8f, 0.3f, 1.0f),
                        "  ! %d 个警告：", (int)warnings.size());
                    ImGui::BeginChild("##属性槽警告列表", ImVec2(0, 70.0f), true);
                    for (const auto& w : warnings)
                        ImGui::BulletText("%s", w.c_str());
                    ImGui::EndChild();
                }
            }
        }
    }

    //渲染通用配置面板（自定义模块）
    void 配置编辑器::渲染通用配置面板()
    {
        if (选中通用配置索引 < 0 ||
            选中通用配置索引 >= (int)仓库.获取通用配置全部().size())
            return;
        通用配置& cfg = 仓库.获取通用配置全部()[选中通用配置索引];
        配置格式* fmt = 仓库.查找格式(cfg.模块名);
        if (fmt == nullptr)
        {
            ImGui::TextWrapped("模块格式不存在（可能已被删除），请检查「工具→配置格式管理」。");
            return;
        }

        // —— 头部 ——
        ImGui::Text("配置：%s / %s", cfg.模块名.c_str(), cfg.条目名.c_str());
        ImGui::TextDisabled("配置文件：%s", cfg.config_path.c_str());
        ImGui::Separator();

        // —— 条目名（通用标识字段）——
        ImGui::Text("条目名（配置标识，保存后用于文件名）");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (输入文本("##通用条目名", cfg.条目名))
            状态消息 = "注意：修改条目名后保存时将自动迁移配置文件";
        ImGui::Separator();

        // —— 按格式字段动态渲染 ——
        渲染通用字段编辑器(cfg, *fmt);

        // —— 操作按钮 ——
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("保存配置", ImVec2(160, 0)))
        {
            std::string error;
            if (仓库.保存通用配置(cfg, error))
                状态消息 = "已保存：" + cfg.config_path;
            else
                状态消息 = "保存失败：" + error;
        }
        ImGui::SameLine();
        bool 正在确认 = (待确认删除条目 == cfg.条目名 && cfg.模块名 == 当前模块);
        if (ImGui::Button(正在确认 ? "再次点击确认删除" : "删除配置", ImVec2(180, 0)))
        {
            if (!正在确认)
                待确认删除条目 = cfg.条目名;
            else
            {
                std::string error;
                if (仓库.删除通用配置(cfg.模块名, cfg.条目名, error))
                {
                    状态消息 = "已删除配置（路由条目）：" + cfg.模块名 + "/" + cfg.条目名;
                    选中通用配置索引 = -1;
                }
                else
                    状态消息 = "删除失败：" + error;
                待确认删除条目.clear();
            }
        }
        if (正在确认)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("取消"))
                待确认删除条目.clear();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                "注意：仅移除路由条目，配置文件保留");
        }

        // —— 校验结果 ——
        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::CollapsingHeader("校验结果"))
        {
            渲染通用校验结果(*fmt, cfg);
        }
    }

    //渲染通用配置的单个字段编辑器
    void 配置编辑器::渲染通用字段编辑器(通用配置& cfg, const 配置格式& fmt)
    {
        for (const auto& f : fmt.字段)
        {
            //确保字段存在（缺失时补默认值）
            if (!cfg.字段值.contains(f.字段名))
                cfg.字段值[f.字段名] = 生成默认值(f);

            //显示名（有显示名用显示名，否则用字段名）
            const char* 标签 = f.显示名.empty() ? f.字段名.c_str() : f.显示名.c_str();

            ImGui::Text("%s", 标签);
            switch (f.类型)
            {
            case 配置字段类型::文本:
            {
                std::string value = cfg.字段值[f.字段名].is_string()
                    ? cfg.字段值[f.字段名].get<std::string>() : std::string();
                std::string label = "##field_" + f.字段名;
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (输入文本(label.c_str(), value))
                    cfg.字段值[f.字段名] = value;
                break;
            }
            case 配置字段类型::脚本路径:
            {
                std::string value = cfg.字段值[f.字段名].is_string()
                    ? cfg.字段值[f.字段名].get<std::string>() : std::string();
                std::string label = "##field_" + f.字段名;
                渲染脚本选择器(label.c_str(), value);
                cfg.字段值[f.字段名] = value;
                break;
            }
            case 配置字段类型::文本列表:
            {
                //从 JSON 读取字符串列表
                std::vector<std::string> list;
                if (cfg.字段值[f.字段名].is_array())
                {
                    for (const auto& item : cfg.字段值[f.字段名])
                        if (item.is_string())
                            list.push_back(item.get<std::string>());
                }

                int 删除索引 = -1;
                for (int i = 0; i < (int)list.size(); ++i)
                {
                    std::string label = "##field_" + f.字段名 + "_" + std::to_string(i);
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 40.0f);
                    输入文本(label.c_str(), list[i]);
                    ImGui::SameLine();
                    if (ImGui::SmallButton((std::string("删除##") + f.字段名 + "_" + std::to_string(i)).c_str()))
                        删除索引 = i;
                }
                if (删除索引 >= 0)
                    list.erase(list.begin() + 删除索引);
                if (ImGui::SmallButton((std::string("+ 添加##") + f.字段名).c_str()))
                    list.push_back(std::string());

                //写回 JSON
                cfg.字段值[f.字段名] = nlohmann::json::array();
                for (const auto& item : list)
                    cfg.字段值[f.字段名].push_back(item);
                break;
            }
            case 配置字段类型::事件对列表:
            {
                //从 JSON 读取二元组列表
                std::vector<std::pair<std::string, std::string>> list;
                if (cfg.字段值[f.字段名].is_array())
                {
                    for (const auto& item : cfg.字段值[f.字段名])
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

                int 删除索引 = -1;
                for (int i = 0; i < (int)list.size(); ++i)
                {
                    float 宽度 = (ImGui::GetContentRegionAvail().x - 60.0f) * 0.5f;
                    ImGui::SetNextItemWidth(宽度);
                    输入文本((std::string("##field_") + f.字段名 + "_cat_" + std::to_string(i)).c_str(),
                        list[i].first);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(宽度);
                    输入文本((std::string("##field_") + f.字段名 + "_tag_" + std::to_string(i)).c_str(),
                        list[i].second);
                    ImGui::SameLine();
                    if (ImGui::SmallButton((std::string("删除##") + f.字段名 + "_" + std::to_string(i)).c_str()))
                        删除索引 = i;
                }
                if (删除索引 >= 0)
                    list.erase(list.begin() + 删除索引);
                if (ImGui::SmallButton((std::string("+ 添加##") + f.字段名).c_str()))
                    list.emplace_back("", "");

                //写回 JSON（两元素数组）
                cfg.字段值[f.字段名] = nlohmann::json::array();
                for (const auto& item : list)
                    cfg.字段值[f.字段名].push_back({ item.first, item.second });
                break;
            }
            case 配置字段类型::整数:
            {
                int value = cfg.字段值[f.字段名].is_number_integer()
                    ? cfg.字段值[f.字段名].get<int>() : 0;
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::InputInt((std::string("##field_") + f.字段名).c_str(), &value))
                    cfg.字段值[f.字段名] = value;
                break;
            }
            case 配置字段类型::浮点数:
            {
                float value = cfg.字段值[f.字段名].is_number()
                    ? cfg.字段值[f.字段名].get<float>() : 0.0f;
                ImGui::SetNextItemWidth(200.0f);
                if (ImGui::InputFloat((std::string("##field_") + f.字段名).c_str(), &value))
                    cfg.字段值[f.字段名] = value;
                break;
            }
            case 配置字段类型::布尔:
            {
                bool value = cfg.字段值[f.字段名].is_boolean()
                    ? cfg.字段值[f.字段名].get<bool>() : false;
                if (ImGui::Checkbox((std::string("##field_") + f.字段名).c_str(), &value))
                    cfg.字段值[f.字段名] = value;
                break;
            }
            }

            //字段说明 + 必填标记
            if (!f.说明.empty() || f.必填)
            {
                std::string 提示 = f.必填 ? "(必填) " : "(可选) ";
                提示 += f.说明;
                ImGui::TextDisabled("%s", 提示.c_str());
            }
            ImGui::Spacing();
        }
    }

    //渲染通用配置校验结果
    void 配置编辑器::渲染通用校验结果(const 配置格式& fmt, const 通用配置& cfg)
    {
        std::vector<std::string> errors, warnings;
        bool 通过 = 仓库.校验通用配置(fmt, cfg, errors, warnings);

        ImGui::Text("校验结果：");
        if (通过 && warnings.empty())
        {
            ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "  ✓ 校验通过");
        }
        else
        {
            if (!errors.empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                    "  ✗ %d 个错误（将导致引擎拒绝加载）：", (int)errors.size());
                ImGui::BeginChild("##通用错误列表", ImVec2(0, 110.0f), true);
                for (const auto& e : errors)
                    ImGui::BulletText("%s", e.c_str());
                ImGui::EndChild();
            }
            if (!warnings.empty())
            {
                ImGui::TextColored(ImVec4(0.95f, 0.8f, 0.3f, 1.0f),
                    "  ! %d 个警告（不影响加载，但建议修复）：", (int)warnings.size());
                ImGui::BeginChild("##通用警告列表", ImVec2(0, 90.0f), true);
                for (const auto& w : warnings)
                    ImGui::BulletText("%s", w.c_str());
                ImGui::EndChild();
            }
        }
    }

    //渲染配置格式管理窗口
    void 配置编辑器::渲染格式管理窗口()
    {
        if (!显示格式管理)
            return;

        //首次打开时若格式集合为空，尝试加载
        if (仓库.获取格式全部().empty())
        {
            仓库.加载格式();
            if (!仓库.获取格式全部().empty())
            {
                格式编辑 = 仓库.获取格式全部().front();
                格式编辑有效 = true;
            }
        }

        //窗口尺寸
        const ImVec2 屏幕 = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowSize(ImVec2(屏幕.x * 0.72f, 屏幕.y * 0.78f), ImGuiCond_Appearing);
        ImGui::SetNextWindowPos(ImVec2(屏幕.x * 0.5f, 屏幕.y * 0.5f),
            ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        //粉色不透明背景（与帮助窗口一致，避免透字）
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.50f, 0.30f, 0.54f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.62f, 0.40f, 0.66f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.72f, 0.38f, 0.68f, 1.00f));

        if (ImGui::Begin("配置格式管理", &显示格式管理, ImGuiWindowFlags_NoCollapse))
        {
            float 左栏宽 = ImGui::GetContentRegionAvail().x * 0.30f;

            // —— 左栏：格式列表 ——
            ImGui::BeginChild("##格式列表", ImVec2(左栏宽, 0), true);
            {
                const auto& 格式集合 = 仓库.获取格式全部();
                for (int i = 0; i < (int)格式集合.size(); ++i)
                {
                    const auto& fmt = 格式集合[i];
                    std::string 标签 = fmt.模块名 + (fmt.内置 ? "（内置）" : "（自定义）");
                    bool selected = 格式编辑有效 && (格式编辑.模块名 == fmt.模块名);
                    if (ImGui::Selectable(标签.c_str(), selected))
                    {
                        格式编辑 = fmt;
                        格式编辑有效 = true;
                    }
                }

                ImGui::Separator();
                ImGui::TextDisabled("内置格式仅可查看，不可编辑/删除");

                // —— 新建模块 ——
                ImGui::Spacing();
                ImGui::Text("新建模块：");
                输入文本提示("##新模块名", 新模块名输入, "模块名（如 Effect_Manager）");
                if (ImGui::Button("创建并添加", ImVec2(-1, 0)))
                {
                    std::string module = 新模块名输入;
                    while (!module.empty() && module.front() == ' ') module.erase(module.begin());
                    while (!module.empty() && module.back() == ' ') module.pop_back();

                    if (module.empty())
                    {
                        状态消息 = "模块名不能为空";
                    }
                    else if (仓库.查找格式(module) != nullptr)
                    {
                        状态消息 = "模块已存在：" + module;
                    }
                    else
                    {
                        //创建新格式：默认目录 custom/<模块名>，路由 custom_<模块名>.json
                        配置格式 fmt;
                        fmt.模块名 = module;
                        fmt.配置目录 = "custom/" + module;
                        fmt.路由文件名 = "custom_" + module + ".json";
                        fmt.内置 = false;
                        //默认字段：type（文本，必填）
                        配置字段定义 type_field;
                        type_field.字段名 = "type";
                        type_field.显示名 = "type（条目标识）";
                        type_field.类型 = 配置字段类型::文本;
                        type_field.必填 = true;
                        type_field.说明 = "条目标识，保存后用于文件名";
                        fmt.字段.push_back(std::move(type_field));

                        std::string error;
                        if (仓库.保存格式(fmt, error))
                        {
                            状态消息 = "已创建模块格式：" + module;
                            格式编辑 = fmt;
                            格式编辑有效 = true;
                            新模块名输入.clear();
                        }
                        else
                            状态消息 = "创建失败：" + error;
                    }
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();

            // —— 右栏：字段编辑 ——
            ImGui::BeginChild("##格式编辑", ImVec2(0, 0), true);
            {
                if (!格式编辑有效)
                {
                    ImGui::TextWrapped("请从左侧选择一个配置格式，或创建新模块。");
                }
                else
                {
                    //模块基本信息
                    ImGui::Text("模块：%s", 格式编辑.模块名.c_str());
                    ImGui::TextDisabled("配置目录：config/%s | 路由：route/%s%s",
                        格式编辑.配置目录.c_str(), 格式编辑.路由文件名.c_str(),
                        格式编辑.内置 ? " | 内置格式" : "");

                    if (格式编辑.内置)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f),
                            "内置格式不可编辑，仅供查看字段定义。");
                        ImGui::Separator();
                    }

                    // —— 字段列表 ——
                    ImGui::Text("字段定义：");
                    int 删除字段索引 = -1;

                    for (int i = 0; i < (int)格式编辑.字段.size(); ++i)
                    {
                        auto& f = 格式编辑.字段[i];
                        std::string prefix = "##fmt_" + std::to_string(i) + "_";

                        //字段名
                        ImGui::Text("字段 %d：", i + 1);
                        if (!格式编辑.内置)
                        {
                            ImGui::SetNextItemWidth(180.0f);
                            输入文本((prefix + "name").c_str(), f.字段名);
                            ImGui::SameLine();
                        }
                        else
                        {
                            ImGui::TextDisabled("%s", f.字段名.c_str());
                            ImGui::SameLine();
                        }

                        //类型下拉
                        if (!格式编辑.内置)
                        {
                            if (ImGui::BeginCombo((prefix + "type").c_str(),
                                实体配置仓库::字段类型名称(f.类型), 0))
                            {
                                for (int t = 0; t <= (int)配置字段类型::布尔; ++t)
                                {
                                    auto type = (配置字段类型)t;
                                    bool selected = (f.类型 == type);
                                    if (ImGui::Selectable(实体配置仓库::字段类型名称(type), selected))
                                        f.类型 = type;
                                }
                                ImGui::EndCombo();
                            }
                            ImGui::SameLine();
                        }
                        else
                        {
                            ImGui::TextDisabled("类型：%s", 实体配置仓库::字段类型名称(f.类型));
                        }

                        //必填勾选
                        if (!格式编辑.内置)
                        {
                            ImGui::Checkbox((prefix + "req").c_str(), &f.必填);
                            ImGui::SameLine();
                            ImGui::TextDisabled("必填");
                        }

                        //说明输入（内置格式仅显示）
                        if (!格式编辑.内置)
                        {
                            输入文本((prefix + "desc").c_str(), f.说明);
                        }
                        else
                        {
                            ImGui::TextDisabled("说明：%s", f.说明.c_str());
                        }

                        //删除字段（内置格式不可删）
                        if (!格式编辑.内置)
                        {
                            if (ImGui::SmallButton((std::string("删除字段##") + std::to_string(i)).c_str()))
                                删除字段索引 = i;
                        }

                        ImGui::Separator();
                    }

                    if (删除字段索引 >= 0)
                        格式编辑.字段.erase(格式编辑.字段.begin() + 删除字段索引);

                    // —— 添加字段区（仅自定义格式）——
                    if (!格式编辑.内置)
                    {
                        ImGui::Spacing();
                        ImGui::Text("添加字段：");
                        ImGui::SetNextItemWidth(160.0f);
                        输入文本提示("##新字段名", 新字段名输入, "字段名");
                        ImGui::SameLine();
                        if (ImGui::BeginCombo("##新字段类型", 实体配置仓库::字段类型名称(新字段类型), 0))
                        {
                            for (int t = 0; t <= (int)配置字段类型::布尔; ++t)
                            {
                                auto type = (配置字段类型)t;
                                bool selected = (新字段类型 == type);
                                if (ImGui::Selectable(实体配置仓库::字段类型名称(type), selected))
                                    新字段类型 = type;
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::SameLine();
                        ImGui::Checkbox("##新字段必填", &新字段必填);
                        ImGui::SameLine();
                        ImGui::TextDisabled("必填");
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        输入文本提示("##新字段说明", 新字段说明输入, "字段说明（可选）");
                        if (ImGui::Button("+ 添加字段", ImVec2(160, 0)))
                        {
                            std::string name = 新字段名输入;
                            while (!name.empty() && name.front() == ' ') name.erase(name.begin());
                            while (!name.empty() && name.back() == ' ') name.pop_back();
                            if (name.empty())
                            {
                                状态消息 = "字段名不能为空";
                            }
                            else
                            {
                                //字段名查重
                                bool 重复 = false;
                                for (const auto& existing : 格式编辑.字段)
                                    if (existing.字段名 == name)
                                    {
                                        重复 = true;
                                        break;
                                    }
                                if (重复)
                                {
                                    状态消息 = "字段已存在：" + name;
                                }
                                else
                                {
                                    配置字段定义 field;
                                    field.字段名 = name;
                                    field.显示名 = name;
                                    field.类型 = 新字段类型;
                                    field.必填 = 新字段必填;
                                    field.说明 = 新字段说明输入;
                                    格式编辑.字段.push_back(std::move(field));
                                    新字段名输入.clear();
                                    新字段说明输入.clear();
                                    状态消息 = "已添加字段：" + name;
                                }
                            }
                        }
                    }

                    // —— 底部操作 ——
                    ImGui::Spacing();
                    ImGui::Separator();
                    if (!格式编辑.内置)
                    {
                        if (ImGui::Button("保存格式", ImVec2(140, 0)))
                            保存格式编辑();

                        ImGui::SameLine();
                        //删除格式（二次确认）
                        bool 正在确认 = (待确认删除类型 == std::string("格式:") + 格式编辑.模块名);
                        if (ImGui::Button(正在确认 ? "再次点击确认删除" : "删除格式", ImVec2(160, 0)))
                        {
                            if (!正在确认)
                                待确认删除类型 = std::string("格式:") + 格式编辑.模块名;
                            else
                            {
                                std::string error;
                                if (仓库.删除格式(格式编辑.模块名, error))
                                {
                                    状态消息 = "已删除格式：" + 格式编辑.模块名;
                                    格式编辑有效 = false;
                                    //若当前浏览/新建模块就是被删模块，回退到第一个可用模块
                                    if (当前模块 == 格式编辑.模块名)
                                        当前模块 = "Entity_Manager";
                                    if (新建模块 == 格式编辑.模块名)
                                        新建模块 = "Entity_Manager";
                                }
                                else
                                    状态消息 = "删除失败：" + error;
                                待确认删除类型.clear();
                            }
                        }
                        if (正在确认)
                        {
                            ImGui::SameLine();
                            if (ImGui::SmallButton("取消"))
                                待确认删除类型.clear();
                            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f),
                                "注意：删除格式不会删除已生成的配置文件");
                        }
                    }

                    // —— 字段类型说明 ——
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.94f, 1.0f), "字段类型说明：");
                    ImGui::TextWrapped("• 文本：普通字符串输入框");
                    ImGui::TextWrapped("• 脚本路径：文本输入 + assets/scripts 下 Lua 脚本下拉选择，保存时校验脚本存在性");
                    ImGui::TextWrapped("• 文本列表：字符串数组（如 acls 权限列表）");
                    ImGui::TextWrapped("• 事件对列表：二元组数组（如 needed_events，每项 = [分类, 标签]）");
                    ImGui::TextWrapped("• 整数 / 浮点数 / 布尔：数值与开关控件");
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleColor(3);
    }

    //把当前格式编辑副本写回仓库（保存到磁盘 + 更新内存）
    void 配置编辑器::保存格式编辑()
    {
        if (!格式编辑有效)
            return;

        //字段名校验：字段名不得为空
        for (const auto& f : 格式编辑.字段)
        {
            if (f.字段名.empty())
            {
                状态消息 = "字段名不能为空，请检查字段定义";
                return;
            }
        }

        std::string error;
        if (仓库.保存格式(格式编辑, error))
            状态消息 = "格式已保存：" + 格式编辑.模块名;
        else
            状态消息 = "保存失败：" + error;
    }

    //渲染状态栏
    void 配置编辑器::渲染状态栏()
    {
        ImGui::Separator();
        //左侧：当前模块数量 + 加载消息
        int 当前数量 = 0;
        if (当前模块 == "Entity_Manager")
            当前数量 = (int)仓库.获取全部().size();
        else if (当前模块 == "Property_Manager")
            当前数量 = (int)仓库.获取属性槽全部().size();
        else
        {
            for (const auto& cfg : 仓库.获取通用配置全部())
                if (cfg.模块名 == 当前模块)
                    当前数量++;
        }
        ImGui::Text("%s 配置数量：%d", 当前模块.c_str(), 当前数量);
        ImGui::SameLine();
        ImGui::TextDisabled("%s", 加载消息.c_str());
        //右侧：状态消息
        if (!状态消息.empty())
        {
            float 可用宽度 = ImGui::GetContentRegionAvail().x;
            float 文本宽度 = ImGui::CalcTextSize(状态消息.c_str()).x;
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - (文本宽度 < 可用宽度 ? 文本宽度 : 可用宽度));
            ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.95f, 1.0f), "%s", 状态消息.c_str());
        }
    }
}
