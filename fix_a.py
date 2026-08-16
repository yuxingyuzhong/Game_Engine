# -*- coding: utf-8 -*-
"""修复 A：帮助图片只从 assets/UI 加载（注释行 1 个反斜杠，代码行 2 个反斜杠）"""
import io

path = r'D:\代码存储\代码仓库\游戏引擎\游戏引擎\src\tools\GUI\Config_Editor\core\配置编辑器.cpp'

with io.open(path, encoding='utf-8', newline='') as f:
    content = f.read()

uses_crlf = '\r\n' in content

# 注释行：E:\图库（1 个反斜杠）；代码行："E:\\图库"（2 个反斜杠，C++ 字符串转义）
old_a = """        //1. 用户指定路径（优先）：E:\\图库\\崩坏星穹铁道\\昔涟\\...
        //   路径含中文，stb 内部 fopen 只认 ANSI，必须转 UTF-16 走 _wfopen + 回调
#ifdef _WIN32
        const char* const 用户图片路径 =
            "E:\\\\图库\\\\崩坏星穹铁道\\\\昔涟\\\\f49a8541bac6bb14f9331217a3262f9b_4646931331178572486.png";
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

variants = [old_a]
if uses_crlf:
    variants.append(old_a.replace('\n', '\r\n'))
ok = False
for v in variants:
    if v in content:
        content = content.replace(v, new_a if not uses_crlf else new_a.replace('\n', '\r\n'), 1)
        ok = True
        break

print('[A]', 'OK' if ok else 'FAIL')

with io.open(path, 'w', encoding='utf-8', newline='') as f:
    f.write(content)
