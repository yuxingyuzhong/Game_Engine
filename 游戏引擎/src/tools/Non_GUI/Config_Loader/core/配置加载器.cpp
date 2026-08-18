#include "src/tools/Non_GUI/Config_Loader/局部命名空间使用.h"
#include "src/tools/Non_GUI/Logging/日志系统.h"

//引擎命名空间
namespace engine
{
    //构造函数
    Config_Loader::Config_Loader()
    {
        //生成权限密钥
        this->acl_key = event_terminal.acl_key_gen();
    }

    //基目录获取
    const path Config_Loader::base_dir_get(void)
    {
        //返回配置基目录
        return Engine_Env::exe_dir_get();
    }

    //目录递归扫描
    void Config_Loader::content_scan(vector<u8string>& receiver)
    {
        //构建实际扫描目录
        path actual_scan_dir = base_dir_get() / scan_content;
        //异常信息记录
        error_code ec;

        //构建递归查找迭代器
        auto it = recursive_directory_iterator(actual_scan_dir, ec);
        //若迭代器创建出现异常
        if (error_out(ec))
            return;

        //递归查找文件
        for (; it != recursive_directory_iterator(); it.increment(ec))
        {
            //若迭代器创建/递增出现异常
            if (error_out(ec))
                continue;

            //检查文件是否可读取
            if (it->is_regular_file(ec))
                //若可读取则记录文件路径
                receiver.push_back(it->path().u8string());

            //若文件查询过程中发生异常
            if (error_out(ec))
                continue;
        }
    }

    //路径安全检查
    bool Config_Loader::skip_safety_inspect(const u8string& config_path)
    {
        //转化变量格式
        path suspect_path = config_path;

        //若未解析出有效路径
        if (suspect_path.begin() == suspect_path.end())
        {
            Log::info("路径无效");
            return false;
        }

        //获取首迭代器
        auto it = suspect_path.begin();
        //若路径开头非"assets/"路径
        if (it->string() != "assets")
        {
            Log::info("路径跳出指定目录: \"assets\"");
            return false;
        }
        //获取第二节迭代器
        it++;
        //若路径第二节非"config/"路径
        if (it->string() != "config")
        {
            Log::info("路径跳出指定目录: \"assets/config\"");
            return false;
        }

        //检查路径中是否存在危险跳转符号".."
        for (const auto& part : suspect_path)
            //若检测出跳转符号
            if (part.string() == "..")
            {
                Log::info("存在父目录返回符号");
                return false;
            }

        //异常信息记录
        error_code ec;
        //拼接绝对路径
        suspect_path = base_dir_get() / suspect_path;
        //若访问路径不存在或发生系统错误
        if (!exists(suspect_path, ec))
        {
            Log::info("访问路径不存在");
            error_out(ec);
            return false;
        }
        //若访问路径非可读取文件
        if (!is_regular_file(suspect_path, ec))
        {
            Log::info("访问路径不可读取");
            error_out(ec);
            return false;
        }

        //若所有检查均通过
        return true;
    }

    //文件读取
    void Config_Loader::file_read(const path& file_path, json& receiver)
    {
        try
        {
            //打开目标文件
            ifstream file(file_path);
            //若文件打开失败
            if (!file.is_open())
            {
                Log::info("Config_Loader::文件打开失败");
                receiver = json();
                return;
            }
            //读取文件内容
            file >> receiver;
        }
        catch (const json::parse_error& e)
        {
            Log::info("Config_Loader::文件格式非法");
            receiver = json();
        }
        catch (const std::exception& e)
        {
            Log::info("Config_Loader::未知错误:");
            Log::info("错误路径如下: {}", file_path.string());
            Log::info("错误信息如下: {}", e.what());
            receiver = json();
        }
    }

    //异常信息输出
    bool Config_Loader::error_out(error_code& error_info)
    {
        //若异常信息不存在
        if (!error_info)
            //返回异常未输出
            return false;
        //若异常信息存在
        else
        {
            //输出异常信息
            cout << error_info << endl;
            //清空异常信息
            error_info.clear();
            //返回异常已输出
            return true;
        }
    }

    //任务执行
    void Config_Loader::act(void)
    {
        //跳转路由记录
        json route_config;
        //扫描目录记录
        vector<u8string> paths{};
        //扫描路由文件（绝对路径）
        content_scan(paths);

        //若扫描结果为空
        if (paths.empty())
        {
            Log::info("Config_Loader::未检测到任何路由文件");
            return;
        }

        for (const auto& path_string : paths)
        {
            //路由配置文件读取路径转换
            path read_path = path_string;
            //读取路由配置文件
            file_read(read_path, route_config);

            //若路由配置文件格式非数组形式
            if (!route_config.is_array())
            {
                Log::info("Config_Loader::当前路由文件内容格式非json数组");
                Log::info("正在读取下一份配置文件");
                continue;
            }

            //读取配置数据
            for (const auto& route_data : route_config)
            {
                //若配置数据不包含指定字段
                if (!route_data.contains("module") || !route_data.contains("config_path"))
                {
                    Log::info("Config_Loader::未包含指定字段:");
                    Log::info("\"module\" and \"config_path\"");
                    continue;
                }
                //若配置数据非字符串
                if (!route_data["module"].is_string() || !route_data["config_path"].is_string())
                {
                    Log::info("Config_Loader::所需字段存储内容异常");
                    Log::info("非所需字符串格式");
                    continue;
                }
                //若配置数据指定字段内容为空
                if ((route_data["module"].empty() || route_data["config_path"].empty()))
                {
                    Log::info("Config_Loader::所需字段无内容");
                    continue;
                }

                //存储目标模块信息
                std::string module = route_data["module"];
                //缓冲跳转路径
                std::string config_path = route_data["config_path"];
                //转换为可用格式
                u8string u8config_path(config_path.begin(), config_path.end());

                //跳转路径安全检查
                if (!skip_safety_inspect(u8config_path))
                {
                    Log::info("Config_Loader::存在恶意跳转");
                    Log::info("跳转路径: {}", config_path); ;
                    continue;
                }

                //格式化为绝对文件路径
                path absolute_config_path = base_dir_get() / u8config_path;
                //配置数据记录
                json config_data;
                //读取配置文件
                file_read(absolute_config_path, config_data);

                //若配置数据读取失败
                if (config_data.is_null())
                {
                    Log::info("Config_Loader::配置读取失败");
                    Log::info("失败路径:{}", absolute_config_path.string());
                    continue;
                }

                //构造配置事件
                shared_ptr<config_event> event(new(nothrow) config_event());
                //若配置事件构造失败
                if (event == nullptr)
                    return;
                //填充事件分类
                event->category = "Config";
                //填充事件标签
                event->tag = "Load";
                //填充目标模块
                event->target_module = module;
                //填充配置数据
                event->config = config_data;
                //发送事件
                if (!event_terminal.event_send({ event }, acl_key))
                {
                    Log::info("Config_Loader::未注册事件中转站依赖");
                    Log::info("配置工作无法完成");
                    return;
                }
                //报告事件信息
                else
                    Log::info("Config_Loader::已发送配置事件: {}",module);
            }
        }
    }
}
