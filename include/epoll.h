/*
 * @epoll.cpp
 * -----------
 * 这是一个基于 epoll 的事件管理类实现文件，适用于高性能网络服务器的 IO 多路复用。
 *
 * 主要功能：
 * - 创建和管理 epoll 实例
 * - 添加、修改、删除监听的文件描述符
 * - 等待事件发生并获取事件信息
 *
 * 类 Epoller 提供如下接口：
 *   Epoller(int MaxEvents)         // 构造函数，指定最大事件数
 *   ~Epoller()                     // 析构函数，关闭 epoll 文件描述符
 *   bool AddFd(int fd, uint32_t events)   // 添加监听的文件描述符及事件
 *   bool ModFd(int fd, uint32_t events)   // 修改监听的事件
 *   bool DelFd(int fd)                    // 删除监听的文件描述符
 *   int  Wait(int timeoutMs)              // 等待事件发生，返回事件数
 *   int  Get_Event_FileD(size_t idx) const    // 获取第 idx 个事件的文件描述符
 *   uint32_t Get_Event_events(size_t idx) const // 获取第 idx 个事件的类型
 *
 * 使用说明：
 * 1. 创建 Epoller 对象，指定最大监听事件数。
 * 2. 使用 AddFd/ModFd/DelFd 管理监听的文件描述符。
 * 3. 调用 Wait 等待事件发生，随后通过 Get_Event_FileD 和 Get_Event_events 获取事件详情。
 *
 * 依赖：
 * - epoll.h 头文件
 * - Linux epoll API
 *
 * 路径：webserve/src/epoll.cpp
 */
#pragma once
#include<sys/epoll.h> //epoll_ctl()
#include<fcntl.h> //fcntl()
#include<unistd.h> //close()
#include<assert.h>
#include<vector>
#include<errno.h>

class Epoller{
    private:
    //Epoll描述符，能管理多个文件描述符，同时监听这些事件，通过epoll_create函数来产生
    int EpollerFd_;
    //被关注的文件发生了指定动作，存在这里
    // struct epoll_event {
    //     uint32_t events;  //描述事件类型
    //     epoll_data_t data; //与事件相关的用户数据
    // };
    // typedef union epoll_data {
    //     void    *ptr;
    //     int      fd; //文件描述符
    //     uint32_t u32;//32位无符号整数
    //     uint64_t u64;//64位无符号整数
    // }    
    std::vector<epoll_event>Events_;

    public:
    ~Epoller();
    //使用 explicit 构造函数避免意外的类型转换(隐式类型转换),创建Epoller时接受一个整型来指示最大事件数量
    explicit Epoller(int MaxEvent=1024);
    //将文件描述符Filed的Events动作加入Epoll监控(Events可以包含一个或多个事件标志)
    bool AddFd(int FileD,uint32_t Events);
    //修改描述符FileD对应的事件
    bool ModFd(int FileD,uint32_t Events);
    //移除对描述符FileD的监控
    bool DelFd(int FileD);
    //使用epoll_wait()等待直到文件描述符上发生指定的事件或超时
    //获得事件对应的描述符(Events_[Index].data.fd)，输入参数表示指定事件在Events_中的索引
    int Get_Event_FileD(size_t Index) const;
    //获得事件对应的类型(Events_[Index].events)
    uint32_t Get_Event_events(size_t Index) const;

    int Wait(int timeoutMs);
};