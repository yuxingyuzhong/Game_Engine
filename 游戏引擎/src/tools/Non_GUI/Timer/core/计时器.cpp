#include "../局部命名空间使用.h"

namespace engine
{
    //构建计时任务
    bool Timer::task_build(const string& task_name)
    {
        //若不存在该计时任务
        if (!task_origin_set.count(task_name))
        {
            task_origin_set.insert({ task_name,Clock::now() });
            return true;
        }
        //若存在该计时任务
        else
            return false;
    }

    //卸载计时任务
    bool Timer::task_unload(const std::string& task_name)
    {
        //获取目标任务迭代器
        auto it = task_origin_set.find(task_name);
        //若目标任务存在则卸载
        if (it != task_origin_set.end())
        {
            task_origin_set.erase(it);
            return true;
        }
        //若目标任务不存在
        else
            return false;
    }

    //获取时间间隔
    Duration Timer::elapsed(const string& task_name, bool restart)
    {
        //获取目标任务迭代器
        auto it = task_origin_set.find(task_name);
        //若目标任务存在且不重置计时器
        if (it != task_origin_set.end() && restart == false)
            return Clock::now() - it->second;
        //若目标任务存在且重置计时器
        else if (it != task_origin_set.end() && restart == true)
        {
            //缓存计时起点
            TimePoint buffer = it->second;
            //重置计时起点
            it->second = Clock::now();
            //返回时间间隔
            return Clock::now() - buffer;
        }
        //若目标任务不存在
        else
            return {};
    }

    //获取秒级时间精度
    double Timer::units(const Duration& sometime)
    {
        return duration<double>(sometime).count();
    }

    //获取毫秒级时间精度
    double Timer::Milli_units(const Duration& sometime)
    {
        return duration<double, milli>(sometime).count();
    }

    //获取微秒级时间精度
    double Timer::Micro_units(const Duration& sometime)
    {
        return duration<double, micro>(sometime).count();
    }

    //获取纳秒级时间精度
    long long Timer::Nano_units(const Duration& sometime)
    {
        return duration_cast<nanoseconds>(sometime).count();
    }

}
