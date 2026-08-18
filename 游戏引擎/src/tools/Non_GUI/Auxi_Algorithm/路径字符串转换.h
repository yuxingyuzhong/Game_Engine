#pragma once
//预编译头
#include "common/前置头文件包含.h"
//获取引擎环境
#include "src/tools/Non_Gui/Engine_Env/引擎环境.h"

//通用算法模块
namespace engine
{
    //路径字符串转化
    inline std::string path_to_string(const std::filesystem::path& path)
    {
        //转化为UTF-8编码的U8字符串
        const std::u8string& u8string = path.u8string();
        //逐字节转化为UTF-8编码的字符串
        std::string string(reinterpret_cast<const char*>(u8string.data()), u8string.size());
        //返回转化结果
        return string;
    }

    //字符串路径转化
    inline std::filesystem::path string_to_path(const std::string& path)
    {
        //转化为UTF-8编码的U8字符串
        std::u8string u8string(reinterpret_cast<const char8_t*>(path.data()), path.size());
        //逐字节转化为UTF-8编码的字符串
        //返回转化结果
        return std::filesystem::path(u8string);
    }
}
