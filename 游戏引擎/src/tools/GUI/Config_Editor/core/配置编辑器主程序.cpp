//============================================================================
// 配置编辑器主程序 —— main() 入口
// ---------------------------------------------------------------------------
// 职责：
//   1. 初始化 GLFW 窗口 + OpenGL 上下文
//   2. 初始化 Dear ImGui（GLFW + OpenGL3 后端）
//   3. 加载中文字体（微软雅黑/黑体，失败时回退默认字体）
//   4. 运行主循环并驱动配置编辑器界面
//============================================================================

#include "common/前置头文件包含.h"
#include "src/tools/GUI/Config_Editor/配置编辑器.h"

#include <random>
#include <cmath>

//引擎命名空间
using namespace engine;

//============================================================================
// 中文字体加载
//============================================================================
namespace
{
    //尝试加载中文字体（按优先级尝试常见系统字体路径）
    //返回成功加载的字体（失败返回 nullptr）
    ImFont* 加载中文字体(float 像素大小)
    {
        ImGuiIO& io = ImGui::GetIO();

        //用 ImFontGlyphRangesBuilder 合并「中文全范围 + 界面用到的特殊符号」：
        //帮助窗口里的 ♪（音符）、ω（希腊小写）、笑脸字符画、校验结果的 ✓/✗ 等
        //都不在 GetGlyphRangesChineseFull() 范围内，不加进去就会渲染成 '?'
        ImFontGlyphRangesBuilder 字形构建器;
        字形构建器.AddRanges(io.Fonts->GetGlyphRangesChineseFull());
        字形构建器.AddChar(0x266A);   //♪ 音符（帮助正文 / 笑脸字符画）
        字形构建器.AddChar(0x03C9);   //ω 希腊小写（笑脸字符画「＾ω＾」）
        字形构建器.AddChar(0x2713);   //✓ 校验通过
        字形构建器.AddChar(0x2717);   //✗ 校验失败
        字形构建器.AddChar(0x2022);   //• 项目符号（帮助正文）
        字形构建器.AddChar(0x2026);   //… 省略号
        字形构建器.AddChar(0x221A);   //√ 根号（兜底符号）
        字形构建器.AddChar(0x00D7);   //× 乘号（兜底符号）
        ImVector<ImWchar> 字形范围;
        字形构建器.BuildRanges(&字形范围);

        //候选字体（按优先级：微软雅黑 → 黑体 → 宋体 → 楷体）
        //微软雅黑符号覆盖最全（♪ ω ✓ ✗ 都有字形），放第一位保证特殊符号可显示
        const char* 候选字体[] =
        {
            "C:/Windows/Fonts/msyh.ttc",      //微软雅黑
            "C:/Windows/Fonts/simhei.ttf",    //黑体
            "C:/Windows/Fonts/simsun.ttc",    //宋体
            "C:/Windows/Fonts/simkai.ttf",    //楷体
        };

        for (const char* 路径 : 候选字体)
        {
            ImFont* font = io.Fonts->AddFontFromFileTTF(路径, 像素大小, nullptr, 字形范围.Data);
            if (font != nullptr)
                return font;
        }

        //全部失败：回退默认字体（无法显示中文）
        return nullptr;
    }

    //========================================================================
    // 梦幻粉色主题（明丽版）
    // -----------------------------------------------------------------------
    // 明丽 + 渐变 + 星光三连：
    //   1. 背景改为粉紫渐变（上紫粉 → 下亮粉），由渲染梦幻背景() 绘制，
    //      因此 WindowBg 置为全透明，让渐变与星光透出来
    //   2. 控件统一提亮：标题栏 / 按钮 / 选中高亮 / 输入框全部更鲜艳明丽
    //   3. 圆角保持柔和，控件观感更可爱
    //========================================================================
    void 应用粉色主题()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // —— 基础背景：全透明（渐变 + 星光由渲染梦幻背景() 绘制）——
        colors[ImGuiCol_WindowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        // Child 面板：亮粉紫 + 更通透的半透明 —— 让背景粉紫渐变与闪烁星光透出来，
        // 面板不再是一片纯色，整体更有「渐变粉」的层次（太不透明会遮住星光）
        colors[ImGuiCol_ChildBg]             = ImVec4(0.72f, 0.48f, 0.80f, 0.32f);
        colors[ImGuiCol_PopupBg]             = ImVec4(0.55f, 0.32f, 0.58f, 0.98f);
        colors[ImGuiCol_MenuBarBg]           = ImVec4(0.62f, 0.36f, 0.64f, 0.95f);
        colors[ImGuiCol_Border]              = ImVec4(1.00f, 0.75f, 0.95f, 0.55f);
        colors[ImGuiCol_BorderShadow]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

        // —— 标题栏（亮玫瑰粉）——
        colors[ImGuiCol_TitleBg]             = ImVec4(0.72f, 0.38f, 0.68f, 1.00f);
        colors[ImGuiCol_TitleBgActive]       = ImVec4(0.85f, 0.48f, 0.80f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]    = ImVec4(0.62f, 0.32f, 0.58f, 0.90f);

        // —— 文本（亮奶白粉）——
        colors[ImGuiCol_Text]                = ImVec4(1.00f, 0.96f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]        = ImVec4(0.88f, 0.74f, 0.90f, 1.00f);

        // —— 输入框背景（提亮的粉紫）——
        colors[ImGuiCol_FrameBg]             = ImVec4(0.55f, 0.33f, 0.58f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.65f, 0.42f, 0.68f, 1.00f);
        colors[ImGuiCol_FrameBgActive]       = ImVec4(0.72f, 0.48f, 0.74f, 1.00f);

        // —— 按钮（樱花粉 → 亮粉）——
        colors[ImGuiCol_Button]              = ImVec4(0.85f, 0.45f, 0.78f, 1.00f);
        colors[ImGuiCol_ButtonHovered]       = ImVec4(0.95f, 0.58f, 0.88f, 1.00f);
        colors[ImGuiCol_ButtonActive]        = ImVec4(0.75f, 0.36f, 0.70f, 1.00f);

        // —— 选中 / 悬停高亮（亮蔷薇粉）——
        colors[ImGuiCol_Header]              = ImVec4(0.80f, 0.45f, 0.74f, 0.95f);
        colors[ImGuiCol_HeaderHovered]       = ImVec4(0.90f, 0.55f, 0.84f, 1.00f);
        colors[ImGuiCol_HeaderActive]        = ImVec4(0.70f, 0.38f, 0.66f, 1.00f);

        // —— 分隔线 / 滚动条（亮粉紫）——
        colors[ImGuiCol_Separator]           = ImVec4(1.00f, 0.68f, 0.92f, 0.60f);
        colors[ImGuiCol_SeparatorHovered]    = ImVec4(1.00f, 0.78f, 0.95f, 0.80f);
        colors[ImGuiCol_SeparatorActive]     = ImVec4(1.00f, 0.85f, 0.98f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]         = ImVec4(0.36f, 0.20f, 0.40f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]       = ImVec4(0.85f, 0.55f, 0.82f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.95f, 0.65f, 0.90f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.78f, 0.95f, 1.00f);

        // —— 输入框光标 / 选中文本（亮粉）——
        colors[ImGuiCol_TextSelectedBg]      = ImVec4(1.00f, 0.65f, 0.92f, 0.55f);
        colors[ImGuiCol_CheckMark]           = ImVec4(1.00f, 0.85f, 0.97f, 1.00f);

        // —— 选项卡（统一风格，提亮）——
        colors[ImGuiCol_Tab]                 = ImVec4(0.68f, 0.40f, 0.66f, 1.00f);
        colors[ImGuiCol_TabHovered]          = ImVec4(0.88f, 0.52f, 0.82f, 1.00f);
        colors[ImGuiCol_TabActive]           = ImVec4(0.82f, 0.46f, 0.76f, 1.00f);
        colors[ImGuiCol_TabUnfocused]        = ImVec4(0.55f, 0.32f, 0.54f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]  = ImVec4(0.66f, 0.40f, 0.64f, 1.00f);

        // —— 圆角与内边距（柔和圆角；加大 FramePadding 让输入框更高更醒目）——
        style.WindowRounding    = 10.0f;
        style.ChildRounding     = 8.0f;
        style.FrameRounding     = 6.0f;
        style.PopupRounding     = 8.0f;
        style.ScrollbarRounding = 10.0f;
        style.GrabRounding      = 8.0f;
        style.TabRounding       = 6.0f;
        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 0.5f;
        style.FramePadding      = ImVec2(10.0f, 6.0f);
        style.WindowPadding     = ImVec2(12.0f, 12.0f);
        style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    }

    //========================================================================
    // 梦幻背景：粉紫渐变 + 闪烁星光
    // -----------------------------------------------------------------------
    // 在 ImGui 背景层绘制：
    //   1. 全屏垂直渐变（上紫粉 → 下亮粉），明丽梦幻
    //   2. 固定种子的星空（每次启动位置一致），每颗星星按正弦波明灭，
    //      大星星带光晕和十字光芒，营造「时隐时现」的闪烁感
    //========================================================================
    void 渲染梦幻背景()
    {
        ImDrawList* 画布 = ImGui::GetBackgroundDrawList();
        const ImVec2 视口 = ImGui::GetIO().DisplaySize;
        if (视口.x <= 0.0f || 视口.y <= 0.0f)
            return;

        // —— 粉紫渐变（上紫粉 → 下亮粉）——
        画布->AddRectFilledMultiColor(
            ImVec2(0, 0), 视口,
            IM_COL32(186, 96, 208, 255),   //左上：紫粉
            IM_COL32(186, 96, 208, 255),   //右上：紫粉
            IM_COL32(252, 168, 214, 255),  //右下：亮粉
            IM_COL32(252, 168, 214, 255)); //左下：亮粉

        // —— 闪烁星光 ——
        struct 星光
        {
            float x, y;        //相对位置（0~1）
            float 半径;        //星核半径
            float 速度;        //闪烁速度
            float 相位;        //闪烁相位（错开明灭节奏）
        };
        static 星光 群星[80];
        static bool 已播种 = false;
        if (!已播种)
        {
            //固定种子：每次启动星空分布一致，便于反复调试观感
            std::mt19937 随机(20260814);
            std::uniform_real_distribution<float> 横(0.01f, 0.99f);
            std::uniform_real_distribution<float> 纵(0.05f, 0.95f);
            std::uniform_real_distribution<float> 径(1.2f, 4.0f);
            std::uniform_real_distribution<float> 速(0.6f, 2.4f);
            std::uniform_real_distribution<float> 相(0.0f, 6.2832f);
            for (auto& 星 : 群星)
            {
                星.x = 横(随机);
                星.y = 纵(随机);
                星.半径 = 径(随机);
                星.速度 = 速(随机);
                星.相位 = 相(随机);
            }
            已播种 = true;
        }

        const float 时刻 = (float)ImGui::GetTime();
        for (const auto& 星 : 群星)
        {
            //亮度：正弦起伏后平方，大部分时间偏暗、偶尔亮起 = 时隐时现
            float 亮度 = 0.5f + 0.5f * std::sinf(时刻 * 星.速度 + 星.相位);
            float alpha = 0.18f + 0.82f * 亮度 * 亮度;
            ImVec2 位置(星.x * 视口.x, 星.y * 视口.y);

            //大星星：柔和光晕（更明显）
            if (星.半径 > 2.2f)
            {
                画布->AddCircleFilled(位置, 星.半径 * 3.6f,
                    IM_COL32(255, 220, 245, (int)(alpha * 60)));
            }
            //星核
            画布->AddCircleFilled(位置, 星.半径,
                IM_COL32(255, 255, 255, (int)(alpha * 255)));

            //大星星：十字光芒
            if (星.半径 > 2.6f)
            {
                float 光芒长 = 星.半径 * 6.0f;
                ImU32 光芒色 = IM_COL32(255, 240, 250, (int)(alpha * 180));
                画布->AddLine(ImVec2(位置.x - 光芒长, 位置.y),
                    ImVec2(位置.x + 光芒长, 位置.y), 光芒色, 1.0f);
                画布->AddLine(ImVec2(位置.x, 位置.y - 光芒长),
                    ImVec2(位置.x, 位置.y + 光芒长), 光芒色, 1.0f);
            }
        }
    }
    //========================================================================
    // GLFW 窗口关闭回调（关闭确认接线）
    // -----------------------------------------------------------------------
    // 用户点窗口 × 时并不直接关闭：先通知编辑器（有未保存修改则弹确认窗），
    // 并拦截默认关闭；编辑器确认退出后由主循环真正关闭窗口。
    //========================================================================
    void 窗口关闭回调(GLFWwindow* window)
    {
        配置编辑器* 编辑器 = static_cast<配置编辑器*>(glfwGetWindowUserPointer(window));
        if (编辑器 != nullptr)
            编辑器->请求关闭窗口();
        //先拦截默认关闭行为，由编辑器决定何时真正退出
        glfwSetWindowShouldClose(window, GLFW_FALSE);
    }
}

//============================================================================
// 主函数
//============================================================================
int main(void)
{
    //控制台输出切换 UTF-8（调试信息）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // —— GLFW 初始化 ——
    if (!glfwInit())
        return -1;

    //请求 OpenGL 3.3 核心上下文
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // —— 窗口尺寸自适应 ——
    // 以主显示器工作区为基准：默认开 92% 大小并最大化，界面比例更大
    const GLFWvidmode* 视频模式 = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int 屏幕宽度 = 视频模式 ? 视频模式->width : 1920;
    int 屏幕高度 = 视频模式 ? 视频模式->height : 1080;
    int 窗口宽度 = (int)(屏幕宽度 * 0.92f);
    int 窗口高度 = (int)(屏幕高度 * 0.92f);

    //创建窗口
    GLFWwindow* window = glfwCreateWindow(窗口宽度, 窗口高度, "配置编辑器", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);   //垂直同步
    glfwMaximizeWindow(window);   //默认最大化，充分利用屏幕空间

    // —— GLAD 加载 OpenGL 函数 ——
    if (!gladLoadGL(glfwGetProcAddress))
    {
        glfwTerminate();
        return -1;
    }

    // —— Dear ImGui 初始化 ——
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    //梦幻粉色主题（在深色基础上覆盖粉紫配色 + 柔和圆角）
    应用粉色主题();

    //绑定 GLFW + OpenGL3 后端
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    //加载中文字体（按屏幕宽度自适应：大屏用更大字号，整体界面更饱满）
    float 字体大小 = (屏幕宽度 >= 1920) ? 23.0f : 20.0f;
    ImFont* 中文字体 = 加载中文字体(字体大小);
    if (中文字体 == nullptr)
    {
        //无中文字体时提示（控制台），界面仍可运行（中文显示为方块）
        std::printf("[ConfigEditor] 警告：未找到中文字体，界面中文可能无法显示。\n");
    }

    // —— 配置编辑器 ——
    配置编辑器 编辑器;
    编辑器.加载();

    //关闭确认接线：窗口 × 按钮先交给编辑器（有未保存修改时弹确认窗）
    glfwSetWindowUserPointer(window, &编辑器);
    glfwSetWindowCloseCallback(window, 窗口关闭回调);

    // —— 主循环 ——
    while (!glfwWindowShouldClose(window))
    {
        //处理窗口事件
        glfwPollEvents();

        //开始 ImGui 新帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //绘制梦幻背景（粉紫渐变 + 闪烁星光）
        渲染梦幻背景();

        //渲染编辑器界面
        编辑器.渲染();

        //编辑器确认退出后，真正关闭窗口
        if (编辑器.应关闭窗口())
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        //提交渲染
        ImGui::Render();

        //清屏并绘制（亮紫粉，与渐变顶部呼应，避免窗口边缘露黑）
        int 显示宽度 = 0, 显示高度 = 0;
        glfwGetFramebufferSize(window, &显示宽度, &显示高度);
        glViewport(0, 0, 显示宽度, 显示高度);
        glClearColor(0.73f, 0.38f, 0.82f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        //交换缓冲
        glfwSwapBuffers(window);
    }

    // —— 清理 ——
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
