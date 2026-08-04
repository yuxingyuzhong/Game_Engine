#pragma once
#include "common/前置头文件包含.h"
#include "src/core/event/Event_Broker/事件中转站通信结构体.h"

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

        //事件发送接口
        std::function<void(std::vector<std::shared_ptr<config_event>>event_set)> event_entry;
    public:
        //事件发送接口注册
        void event_entry_sign(std::function<void(std::vector<std::shared_ptr<config_event>>event_set)> cb);
        //可执行文件目录注册
        void exe_path_sign(const std::filesystem::path& exe_path);
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