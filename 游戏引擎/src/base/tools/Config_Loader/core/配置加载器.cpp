#include "src/base/tools/Config_Loader/局部命名空间使用.h"

namespace fs = std::filesystem;

//目录递归扫描
void Config_Loader::content_scan(vector<string>& receiver)
{
    //错误信息记录
    error_code ec;

    // 递归目录迭代器
    for (auto it = fs::recursive_directory_iterator(scan_content, ec);
        it != fs::recursive_directory_iterator();
        ++it)
    {
        if (ec) 
        {
            //发生错误时（如权限不足），跳过当前条目并清除错误状态
            ec.clear();
            continue;
        }

        // 只收集普通文件（过滤掉目录、符号链接等）
        if (it->is_regular_file(ec)) 
            receiver.push_back(it->path().string());
        // 如果检查时出错也跳过
        if (ec) 
            ec.clear();
    }

}

//文件跳转检测
bool Config_Loader::skip_exam(const string& path)
{
    //禁止非指定前缀
    if (path.rfind(skip_root_content, 0) != 0)
        return false;
    //禁止上级目录跳转
    else if(path.find("..") != string::npos)
        return false;
    //其余情况通过
    else
        return true;
}

//文件读取
void Config_Loader::file_read(const string& path, json& receiver)
{
    try
    {
        //打开指定文件
        ifstream file(path);
        //若文件打开失败
        if (!file.is_open())
        {
            cout << "Config_Loader::文件打开失败\n";
            //返回空JSON
            receiver = json();   
            return;
        }
        //读取文件内容并解析
        file >> receiver;
    }
    //若文件内容非合法json
    catch (const json::parse_error& e)
    {
        cout << "Config_Loader::文件格式非法\n";
        //返回空JSON
        receiver = json();
    }
    catch (const std::exception& e)
    {
        cout << "Config_Loader::未知错误\n" << path.c_str() << e.what();
        //返回空JSON
        receiver = json();
    }
}

//任务执行
void Config_Loader::act(void)
{
    //若未注册事件中转站依赖
    if (!send)
    {
        cout << "Config_Loader::未注册事件中转站依赖\n无法工作\n";
        return;
    }
    //路由文件内容记录
    json route_json;
    //路由文件路径记录
    vector<string> paths{};
    //扫描获取路由文件路径
    content_scan(paths);
    //循环读取路由文件
    for (int read_time = 0; read_time < paths.size(); read_time++)
    {
        //读取路由文件
        file_read(paths[read_time], route_json);
        //若路由文件格式非法
        if (!route_json.is_array())
        {
            cout << "Config_Loader::路由文件内容格式非法\n";
            continue;
        }

        //遍历文件内路由条目
        for (const auto& entry : route_json)
        {
            // 字段完整性检查
            if (!entry.contains("module") || !entry.contains("config_path"))
            {
                cout << "Config_Loader::文件跳转所需字段未包含\n";
                continue;
            }

            //模块标签记录
            string module = entry["module"];
            //配置路径记录
            string config_path = entry["config_path"];

            //若存在字段为空
            if (module.empty() || config_path.empty())
            {
                cout<< "Config_Loader::所需字段无内容\n";
                continue;
            }

            //若安全检查未通过
            if (!skip_exam(config_path))
            {
                cout<< "Config_Loader::存在恶意跳转\n" << config_path.c_str() << endl;
                continue;
            }

            //配置文件信息存储
            json config_data;
            //读取配置文件
            file_read(config_path, config_data);

            //若读取失败
            if (config_data.is_null())
            {
                cout << "Config_Loader::配置读取失败\n" << config_path.c_str() << endl;
                continue;
            }

            //构建事件
            config_event* evt = new config_event();
            //标明事件类型
            evt->category = "Config";
            evt->tag = "Load";
            //指定发送目标
            evt->target_module = module;
            //记录json包参数
            evt->config = config_data;
            //发送事件
            send({ evt });
        }
    }
}

//事件发送接口注册
void Config_Loader::sign_event_entry(function<void(vector<config_event*>event_set)> cb)
{
    //注册事件发送接口
    send = cb;
}