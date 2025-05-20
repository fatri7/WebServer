/*
 * @timer.cpp
 * -----------
 * 这是一个高效的定时器管理类实现文件，适用于高性能网络服务器的定时任务调度。
 *
 * 主要功能：
 * - 管理定时器的添加、删除、更新和触发
 * - 支持定时任务的回调处理
 * - 使用小根堆实现定时器优先级队列，保证高效的超时检测
 *
 * 类 TimerManager 提供如下接口：
 *   void add_timer(int id, int timeout, const timeout_callback& cb) // 添加或更新定时器
 *   void work(int id)                                               // 触发指定定时器回调并删除
 *   void update(int id, int timeout)                                // 更新定时器超时时间
 *   void handle_expired_event()                                     // 处理所有已到期定时器
 *   void clear()                                                    // 清空所有定时器
 *   int  get_next_timer_handle()                                    // 获取下一个定时器剩余时间
 *
 * 使用说明：
 * 1. 调用 add_timer 添加定时任务，指定唯一 id、超时时间和回调函数。
 * 2. 定期调用 handle_expired_event 检查并处理到期定时器。
 * 3. 可通过 update 更新定时器，或 work 立即触发并删除定时器。
 * 4. get_next_timer_handle 可用于 epoll 等待超时时间的动态调整。
 *
 * 依赖：
 * - timer.h 头文件
 * - C++11 chrono 库
 *
 * 路径：webserve/src/timer.cpp
 */
#pragma once
#include"HttpConnection.h"
#include<queue>
#include<deque>
#include<unordered_map>
#include<ctime>
#include<chrono>
#include<functional>
#include<memory>
/*std::function<void()> 是一个通用的多态函数包装器，
可以存储、复制和调用任何可调用目标，只要该目标可以转换为 void() 类型。
这种类型通常用于回调函数，特别是在设置超时事件时，
当超时发生时，可以调用存储的函数*/
typedef std::function<void()> timeout_callback;
//std::chrono::high_resolution_clock 是 C++ 标准库中的一个时钟类，提供最高精度的计时功能
typedef std::chrono::high_resolution_clock hr_clock;
/*std::chrono::milliseconds 是 C++ 标准库中的一个时长类，表示毫秒级的时间间隔。
这种类型用于表示或计算以毫秒为单位的时间长度*/
typedef std::chrono::milliseconds ms;
/*表示一个特定的时间点。
这种类型通常用于记录事件发生的时间，例如超时事件的触发时间。*/
typedef hr_clock::time_point time_stamp;

struct timer_node{
    //标记
    int id;
    //过期时间
    time_stamp expire;
    //回调函数,方便删除定时器时将对应的HTTP连接关闭
    timeout_callback call_back; 
    //<重载
    bool operator<(const timer_node& t)
    {
        return expire<t.expire;
    }
};
class TimerManager{
    //std::shared_ptr：这是一个智能指针，用于管理共享所有权资源的内存。当没有更多的shared_ptr指向某个对象时，该对象会被自动删除。
    typedef std::shared_ptr<timer_node> shared_timer_node;
    private:
    //删除指定定时器
    void del_(size_t index);
    //向上调整
    void siftup_(size_t index);
    //向下调整
    bool siftdown_(size_t index,size_t n);
    //交换两个结点位置
    void swap_node_(size_t index1,size_t index2);

    //存储定时器的存储实体
    std::vector<timer_node>heap_;
    //映射一个fd对应的定时器在heap_中的位置
    std::unordered_map<int,size_t>ref_;
    public:
    TimerManager(){
        //预分配容量
        heap_.reserve(64);
    };
    ~TimerManager(){
        clear();
    }
    //清空容器
    void clear();
    //设置定时器
    void add_timer(int id,int timeout,const timeout_callback& call_back);
    //清除超时结点
    void handle_expired_event();
    //下一个定时器事件距离当前时间的毫秒数
    int get_next_timer_handle();
    //调整指定id的结点 
    void update(int id,int timeout);
    //删除制定id节点，并且用指针触发处理函数
    void work(int id);
    //从管理器中移除一个定时器
    void pop();
};