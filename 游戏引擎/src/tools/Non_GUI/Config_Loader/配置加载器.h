#pragma once
//预编译头
#include "common/前置头文件包含.h"

// ———— 事件相关 ————

//获取预定义事件类型
#include "common/types/事件类型.h"
//获取事件终端
#include "src/core/event/Event_Terminal/事件终端.h"

// ———— 工具相关 ————

//获取引擎环境信息
#include "src/tools/Non_GUI/Engine_Env/引擎环境.h"

//游戏引擎命名空间
namespace engine
{
    //配置加载器
    class Config_Loader
    {
    private:
        //扫描子目录（相对于基目录）
        std::u8string scan_content = u8"assets/config/route/";
        //允许的根目录（相对于基目录）
        std::u8string allowed_root = u8"assets/config/";
    public:
        //事件终端
        Event_Terminal event_terminal;
    private:
        //权限密钥
        int64_t acl_key = 0;

    public:
        //构造函数
        Config_Loader();

        //析构函数
        ~Config_Loader() = default;

        //任务执行
        void act(void);
    private:
        //基目录获取
        const std::filesystem::path base_dir_get(void);

        //目录递归扫描
        void content_scan(std::vector<std::u8string>& receiver);

        //路径安全检查
        bool skip_safety_inspect(const std::u8string& config_path);

        //文件读取
        void file_read(const std::filesystem::path& file_path, nlohmann::json& receiver);

        //异常信息输出
        bool error_out(std::error_code& info);
    };
}