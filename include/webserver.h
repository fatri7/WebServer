/**
 * @file webserver.h
 * @brief WebServe - 基于 epoll 和线程池的高性能 HTTP 服务器
 *
 * 该头文件定义了 WebServe 类，封装了基于 epoll 的多路复用、定时器管理、线程池任务调度和 HTTP 连接处理等功能。
 *
 * ## 主要特性
 * - 支持高并发的 HTTP 连接管理
 * - 使用 epoll 进行高效的 IO 事件监听
 * - 线程池处理请求，提升并发性能
 * - 支持连接定时关闭，防止资源泄漏
 * - 支持自定义事件触发模式（边缘/水平触发）
 *
 * ## 主要成员
 * - `init_socket_()`：初始化监听套接字
 * - `add_client_connection_()`：添加客户端连接
 * - `close_connection_()`：关闭客户端连接
 * - `handle_listen_()`、`handle_read_()`、`handle_write_()`：处理各类 epoll 事件
 * - `on_read_()`、`on_write_()`、`on_process_()`：回调处理客户端请求
 * - `send_error_()`：发送错误响应
 * - `extent_time_()`：延长连接定时器
 *
 * ## 使用方法
 * 1. 创建 WebServe 实例，传入端口、触发模式、超时时间、延迟关闭选项、线程数等参数
 * 2. 调用 `start()` 启动服务器
 *
 * ## 依赖
 * - epoll.h
 * - timer.h
 * - ThreadPool.h
 * - HttpConnection.h
 * @date 2025
 */
#pragma once
#include"epoll.h"
#include"timer.h"
#include"ThreadPool.h"
#include"HttpConnection.h"

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class WebServe{
    private:
    //初始化套接字
    bool init_socket_();
    
    //初始化事件模式，如边缘触发或水平触发
    void init_event_mode_(int trig_mode);
    
    //添加客户端连接
    void add_client_connection_(int fd,sockaddr_in addr);
    //关闭客户端连接
    void close_connection_(HttpConnection* client);

    //处理监听事件
    void handle_listen_();
    //处理写事件
    void handle_write_(HttpConnection* client);
    //处理读事件
    void handle_read_(HttpConnection* client);

    //读事件回调
    void on_read_(HttpConnection* client);
    //写事件回调
    void on_write_(HttpConnection* client);
    //处理客户端请求
    void on_process_(HttpConnection* client);

    //发送错误信息
    void send_error_(int fd,const char* information);
    //延长客户端连接的定时器
    void extent_time_(HttpConnection* client);

    static const int max_fd_=65536;
    //设置文件描述符为非阻塞模式
    static int set_fd_nonblock_(int fd);

    //服务器监听的端口号
    int port_;    
    //是否开启套接字延迟关闭功能
    bool open_linger_;
    //超时时间，单位为毫秒
    int time_out_ms_;
    //标记服务器是否关闭
    bool close_or_not_;
    //监听套接字的文件描述符
    int listen_fd_;
    //服务器资源目录的路径
    char* srcDir_;

    //监听套接字的事件类型
    uint32_t listen_event_;
    //连接套接字的事件类型
    uint32_t connection_event_;
    
    //定时器管理器，用于处理超时事件
    std::unique_ptr<TimerManager>timer_;
    //线程池，用于处理任务
    std::unique_ptr<CoroutineThreadPool> m_threadpool_;
    //epoll 实例，用于IO多路复用
    std::unique_ptr<Epoller>epoller_;
    //存储所有客户端连接的映射
    std::unordered_map<int,HttpConnection>users_;

    public:
    WebServe(int port,int trig_mode,int timeout_ms,bool opt_linger,int thread_number);
    ~WebServe();
    void start();
};