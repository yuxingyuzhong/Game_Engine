#pragma once
#include "common/前置头文件包含.h"
#include "common/公共命名空间使用.h"
#include "src/base/event/Event_Broker/事件中转站通信结构体.h"

//游戏引擎命名空间
namespace Game_Engine
{
	//配置加载器
	class Config_Loader
	{
	private:
		//扫描目录
		string scan_content = "config/route/";
		//跳转根目录
		string skip_root_content = "config/";

		//事件发送接口
		function<void(vector<config_event*>event_set)> send;
	public:
		//事件发送接口注册
		void sign_event_entry(function<void(vector<config_event*>event_set)> cb);
		//任务执行
		void act(void);
	private:
		//目录递归扫描
		void content_scan(vector<string>& receiver);
		//文件跳转检测
		bool skip_exam(const string& path);
		//文件读取
		void file_read(const string& path,json& receiver);
	};
}