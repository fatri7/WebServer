/*
 * @HttpRequest.cpp
 * ----------------
 * 这是一个 HTTP 请求解析类的实现文件，适用于 Web 服务器对 HTTP 请求的处理。
 *
 * 主要功能：
 * - 解析 HTTP 请求行、请求头和请求体
 * - 支持 GET 和 POST 请求
 * - 处理 URL 编码的表单数据
 * - 路径和方法的提取与规范化
 * - 支持 keep-alive 连接检测
 *
 * 类 HttpRequest 提供如下接口：
 *   void Init()                         // 初始化请求对象，重置状态
 *   bool Parse(Buffer& Buff)            // 解析缓冲区中的 HTTP 请求
 *   std::string Path() const            // 获取请求路径
 *   std::string Method() const          // 获取请求方法
 *   std::string Version() const         // 获取 HTTP 版本
 *   std::string Get_Post(const std::string& key) const // 获取 POST 表单字段
 *   bool Are_You_Keep_Alive() const     // 检查是否为 keep-alive 连接
 *
 * 使用说明：
 * 1. 创建 HttpRequest 对象并调用 Init() 初始化。
 * 2. 调用 Parse 解析 Buffer 中的 HTTP 请求数据。
 * 3. 通过 Path、Method、Version、Get_Post 等接口获取请求信息。
 *
 * 依赖：
 * - <regex>、<unordered_set>、<string>、<map>、<cassert> 等标准库
 * - Buffer 类（用于管理网络数据缓冲区）
 *
 * 路径：webserve/src/HttpRequest.cpp
 */
#pragma once
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include "buffer.h"
class HttpRequest{
    public:
    //HTTP的请求信息,枚举类型，表示状态的变化
    enum PARSE_STATE{
        //解析请求行状态，默认０
        REQUEST_LINE,
        //解析请求头部状态，默认１
        HEADERS,
        //解析请求体状态，默认２
        BODY,
        //解析完成状态，默认３
        FINISH,
    };
    //与 HTTP 请求处理相关的状态码
    enum HTTP_CODE {
        //表示没有接收到有效的 HTTP 请求
        NO_REQUEST = 0,
        //表示接收到一个有效的 GET 请求
        GET_REQUEST,
        //表示接收到一个格式错误或无法理解的请求
        BAD_REQUEST,
        //表示请求的资源不存在
        NO_RESOURSE,
        //表示请求被禁止，通常是因为权限问题
        FORBIDDENT_REQUEST,
        //表示请求的是一个文件
        FILE_REQUEST,
        //表示服务器内部发生错误，无法完成请求
        INTERNAL_ERROR,
        //表示连接已经关闭
        CLOSED_CONNECTION,
    };
    private:
    //当前状态
    PARSE_STATE State_;
    //请求方法、请求路径、HTTP 版本、请求体
    std::string Method_,Path_,Version_,Body_;
    //unordered_map<std::string,std::string>键值对，键是唯一的，键值都是string
    //headers["Host"]//键="www.example.com"//值;
    //请求头映射
    std::unordered_map<std::string,std::string>Header_;
    //POST数据映射，post["username"]="agedcat";
    std::unordered_map<std::string,std::string>Post_;
    //默认HTML文件集合，无序不重复集合（unordered_set）
    static const std::unordered_set<std::string>DEFAULT_HTML_;

    //解析请求行，使用正则表达式来匹配和提取请求行中的方法、路径和版本信息
    bool Parse_Request_Line_(const std::string& Line);
    //解析 HTTP 请求头部
    void Parse_Reauest_Header_(const std::string& Line);
    //解析 HTTP 请求体
    void Parse_Data_Body_(const std::string& Line);

    //解析路径，将简写的路径扩展为完整的 HTML 文件路径
    void Parse_Path_();
    //解析Post数据
    void Parse_Post_();

    //将字符转换为十六进制数
    static int Convert_Hex(char ch);
    public:
    //初始化函数，构造时使用
    void Init();
    //缓存Http请求信息
    bool Parse(Buffer& buff);
    //
    std::string Path() const;
    std::string& Path();
    std::string Method() const;
    
    std::string Version() const;
    //根据键获取 POST 数据
    std::string Get_Post(const std::string& key) const;
    //根据键获取 POST 数据
    std::string Get_Post(const char* key) const;
    //判断Http连接是否alive
    bool Are_You_Keep_Alive() const;
};