/*
 * @HttpConnection.cpp
 * -------------------
 * 这是一个 HTTP 连接管理类的实现文件，适用于高性能 Web 服务器的连接处理。
 *
 * 主要功能：
 * - 管理单个客户端 HTTP 连接的生命周期
 * - 负责连接的初始化、关闭、读写缓冲区管理
 * - 解析 HTTP 请求并生成 HTTP 响应
 * - 支持长连接（keep-alive）和文件映射响应
 *
 * 类 HttpConnection 提供如下接口：
 *   HttpConnection()                        // 构造函数，初始化连接状态
 *   ~HttpConnection()                       // 析构函数，关闭连接
 *   void init_httpconnection(int fd, const sockaddr_in& addr) // 初始化连接
 *   void close_httpconnection()             // 关闭连接并释放资源
 *   int get_Fd() const                      // 获取连接的文件描述符
 *   struct sockaddr_in get_addr() const     // 获取客户端地址
 *   const char* get_ip() const              // 获取客户端 IP 字符串
 *   int get_port() const                    // 获取客户端端口
 *   ssize_t read_buffer(int* save_erron)    // 从连接读取数据到缓冲区
 *   ssize_t write_buffer(int* save_erron)   // 将缓冲区数据写入连接
 *   int get_write_length()                  // 获取待写入数据长度
 *   bool get_alive_status() const           // 判断连接是否为长连接
 *   bool handle_httpconnection()            // 处理 HTTP 请求并生成响应
 *
 * 使用说明：
 * 1. 创建 HttpConnection 对象，调用 init_httpconnection 初始化连接。
 * 2. 使用 read_buffer/write_buffer 进行数据收发。
 * 3. 调用 handle_httpconnection 解析请求并生成响应。
 * 4. 连接结束时调用 close_httpconnection 释放资源。
 *
 * 依赖：
 * - sys/socket.h, netinet/in.h, unistd.h, sys/uio.h 等头文件
 * - 相关 HTTP 请求/响应解析与缓冲区管理类
 *
 * 路径：webserve/src/HttpConnection.cpp
 */
#pragma once
#include"buffer.h"
#include"HttpResponse.h"
#include"HttpRequest.h"

#include<arpa/inet.h> //sockaddr_in
#include<sys/uio.h> //readv/writev
#include<iostream>
#include<sys/types.h>
#include<assert.h>

class HttpConnection{
    private:
    //连接的描述符
    int fd_;
    /*通常用于socket编程中存储服务端或客户端的地址信息
    sin_family：地址族，对于IPv4地址，这个值通常是 AF_INET。
    sin_port：端口号，使用网络字节顺序（大端序）。
    sin_addr：IP地址，是一个 in_addr 结构体，包含一个名为 s_addr 的成员，用于存储IP地址。
    sin_zero：填充字段，用于将 sockaddr_in 结构体的大小与 sockaddr 结构体对齐。*/
    struct sockaddr_in addr_;
    //标记是否关闭连接
    bool close_or_not;
    //用于存储iovec结构体数组的数量
    int iov_count_;
    //用于存储分散/聚集I/O操作的数据块信息
    struct iovec iov_[2];
    Buffer read_buffer_;
    Buffer write_buffer_;
    HttpRequest request_;
    HttpResponse response_;

    public:
    HttpConnection();
    ~HttpConnection();
    void init_httpconnection(int socketFd,const sockaddr_in& addr);
    ////每个连接中定义的对缓冲区的读接口
    ssize_t read_buffer(int* save_errono);
    ////每个连接中定义的对缓冲区的写接口
    ssize_t write_buffer(int* save_errono);
    //关闭HTTP连接
    void close_httpconnection();
    //处理HTTP连接，主要分为request的解析和response的生成
    bool handle_httpconnection();
    
    //获得IP
    const char* get_ip() const;
    //获得port
    int get_port() const;
    //获得连接的描述符
    int get_Fd() const;
    //获得存储服务端或客户端的地址信息
    sockaddr_in get_addr() const;
    //获得要写入的长度
    int get_write_length();
    //获得是否保持连接的判断
    bool get_alive_status() const;
    //标记是否使用边缘触发
    static bool isEt;
    static const char* srcDir;
    static std::atomic<size_t>user_count;
    
};