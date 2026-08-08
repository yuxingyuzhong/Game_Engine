#pragma once
#include "common/前置头文件包含.h"
#include "common/types/事件类型.h"
#include "src/core/event/Event_Terminal/事件终端.h"

//游戏引擎命名空间
namespace engine
{
    //配置加载器
    class Config_Loader
    {
    private:
        //可执行文件目录（绝对路径）
        std::filesystem::path exe_path{};
        //基目录（可执行文件所在目录）
        std::filesystem::path base_path{};
        //扫描子目录（相对于基目录）
        std::u8string scan_content = u8"config/route/";
        //允许的根目录（相对于基目录）
        std::u8string allowed_root = u8"config/";
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

        //可执行文件目录注册
        void exe_path_register(const std::filesystem::path& exe_path);

        //任务执行
        void act(void);
    private:
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