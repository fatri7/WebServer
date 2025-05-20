/*
 * @HttpResponse.cpp
 * -----------------
 * 这是一个 HTTP 响应处理类的实现文件，适用于 Web 服务器的响应生成与文件映射。
 *
 * 主要功能：
 * - 生成标准 HTTP 响应报文（状态行、响应头、响应体）
 * - 支持常见 MIME 类型和错误页面映射
 * - 文件内容高效映射到内存（mmap），提升静态资源访问性能
 * - 支持 Keep-Alive 长连接
 * - 错误处理与自定义错误页面
 *
 * 类 HttpResponse 提供如下接口：
 *   HttpResponse()                              // 构造函数，初始化成员
 *   ~HttpResponse()                             // 析构函数，释放映射文件资源
 *   void Init(const std::string& srcDir, std::string& path, bool keepAlive, int code)
 *                                               // 初始化响应参数
 *   void make_Response(Buffer& buffer)          // 生成完整 HTTP 响应写入 buffer
 *   char* file()                               // 获取映射文件指针
 *   size_t file_Length() const                 // 获取映射文件长度
 *
 * 内部机制：
 * - 根据请求路径和状态码选择响应文件
 * - 自动判断文件类型并设置 Content-Type
 * - 通过 mmap 映射文件，提升大文件传输效率
 * - 支持 200、400、403、404 等常见 HTTP 状态码
 *
 * 使用说明：
 * 1. 调用 Init 设置响应参数（目录、路径、是否长连接、状态码）。
 * 2. 调用 make_Response 生成响应内容到 Buffer。
 * 3. 通过 file() 和 file_Length() 获取文件内容用于高效发送。
 *
 * 依赖：
 * - C++ STL
 * - Linux 系统调用（stat, open, mmap, munmap 等）
 * - Buffer 类用于数据写入
 *
 * 路径：webserve/src/HttpResponse.cpp
 */
#pragma once
#include <unordered_map>
#include <fcntl.h>  //open
#include <unistd.h> //close
#include <sys/stat.h> //stat
#include <sys/mman.h> //mmap,munmap
#include <assert.h>
#include "buffer.h"
class HttpResponse{
    private:
    //HTTP响应状态码
    int code_;
    //表示是否保持连接
    bool Are_You_Keep_Alive_;
    
    //请求的文件路径
    std::string path_;
    //资源目录
    std::string srcDir_;
    
    //内存映射的文件指针
    char* mmFile_;
    //文件状态信息，系统结构体，包含了文件的各种属性，如大小、权限、时间戳等
    struct stat mmFileStat_;
    
    //文件后缀与MIME类型的映射
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    //状态码与状态信息的映射
    static const std::unordered_map<int, std::string> CODE_STATUS;
    //状态码与错误页面路径的映射
    static const std::unordered_map<int, std::string> CODE_PATH;
    
    //添加状态行到缓冲区
    void add_State_Line_(Buffer& buffer);
    //添加响应头到缓冲区
    void add_Response_Header_(Buffer& buffer);
    //添加响应内容到缓冲区
    void add_Response_Content_(Buffer& buffer);

    //生成错误HTML页面
    void errorHTML_();
    //获取文件类型
    std::string get_File_Type();

    public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& srcDir,std::string& path_,bool Are_You_Keep_Alive=false,int code=-1);
    //生成HTTP响应
    void make_Response(Buffer& buffer);
    //解除文件映射
    void unmap_File();
    //获取映射文件的指针
    char* file();
    //获取文件长度
    size_t file_Length() const;
    //生成错误响应内容
    void error_Content(Buffer& buffer,std::string message);
    //获取响应状态码
    int code()const{
        return code_;
    }
};