/*
 * @file buffer.cpp
 * @brief 实现了 Buffer 类，用于高效管理和操作内存缓冲区，支持读写指针、自动扩容、与文件描述符的数据交互等功能。
 *
 * 主要功能：
 * - 管理内部缓冲区，实现高效的读写操作
 * - 自动扩容与空间复用，避免频繁分配内存
 * - 支持与文件描述符的数据读写（readv/write）
 * - 提供便捷的字符串与二进制数据接口
 *
 * 类 Buffer 提供如下接口：
 *   Buffer(int initBuffersize)                 // 构造函数，指定初始缓冲区大小
 *   void Init_Buffer()                         // 重置缓冲区内容和指针
 *   void Write_to_Buffer(const char*, size_t)  // 写入数据到缓冲区
 *   void Write_to_Buffer(const std::string&)   // 写入字符串到缓冲区
 *   void Write_to_Buffer(const void*, size_t)  // 写入任意数据到缓冲区
 *   void Write_to_Buffer(const Buffer&)        // 写入另一个 Buffer 的数据
 *   ssize_t Get_Data(int fd, int* Errno)       // 从文件描述符读取数据
 *   ssize_t Put_Data(int fd, int* Errno)       // 向文件描述符写入数据
 *   std::string Get_All_Data_String()          // 获取所有未读数据并清空缓冲区
 *
 * 使用说明：
 * 1. 创建 Buffer 对象，指定初始大小。
 * 2. 使用 Write_to_Buffer 写入数据，或 Get_Data 从 fd 读取。
 * 3. 使用 Put_Data 写出数据，或 Get_All_Data_String 获取全部内容。
 *
 * 依赖：
 * - cstring, vector, string, unistd.h, sys/uio.h
 *
 * 路径：webserve/src/buffer.cpp
 */

#pragma once
#include<vector>
#include<iostream>
#include<cstring>
#include<atomic>
#include<unistd.h> //read() write()
#include<sys/uio.h> //readv() writev()
#include<assert.h>
class Buffer{
    private:
    //缓冲区实体(数据)
    std::vector<char>buffer_;
    //“读指针”(读位置在缓冲区实体内部的正整数索引)
    std::atomic<size_t>ReadPos_;
    //“写指针”(写位置在缓冲区实体内部的正整数索引)
    std::atomic<size_t>WritePos_;
    //获得缓冲区的起始位置
    char* BeginPtr_();
    //获得缓冲区的起始位置(只读)
    const char* BeginPtr_() const;
    //需要写入新数据时判断现有缓冲区空间是否足够，否则扩容
    void We_Should_Be_Enough_(size_t Length_We_Need);
    
    public:
    //在创建一个缓存区对象时需指定初始大小(默认1024)
    Buffer(int InitBufferSpace=1024);
    //初始化当前缓存区(包括内容、读写指针位置)
    void Init_Buffer();

    //返回现有缓存区未写入数据的字节数
    size_t How_Many_Bytes_Can_We_Write() const;
    //返回已写入的内容中待读取的字节数
    size_t How_Many_Bytes_We_Need_Read() const;
    
    //返回当前读指针
    const char* Where_Did_We_Read() const;
    //更新读指针(指定前进距离)
    void Update_ReadPos(size_t Step_Length);
    //更新读指针(指定目标位置)
    void Update_ReadPos(const char* Destination);

    //返回当前写指针
    char* Where_Did_We_Write(); 
    //返回当前写指针(只读)
    const char* Where_Did_We_Write_Const() const;
    //写入前的确认工作(剩余空间是否足够)
    void Do_We_Have_Enough_Spase(size_t Lengh_We_Need);
    //写入缓存区(参数:char*字符串指针及其长度)对写入内容只读
    void Write_to_Buffer(const char* Data,size_t Data_Length);
    //写入缓存区(参数:string字符串引用)对写入内容只读
    void Write_to_Buffer(const std::string& Data);
    //写入缓存区(参数:void*数据指针及其长度)对写入内容只读
    void Write_to_Buffer(const void* Data,size_t Data_Length);
    //写入缓存区(参数:其余Buffer实体引用)对写入内容只读
    void Write_to_Buffer(const Buffer& Data_of_Other_Buffer);
    
    //系统io功能:从文件(FileD)读入Buffer
    ssize_t Get_Data(int FileD,int* Errono);
    //系统io功能:从Buffer写入文件(FileD)
    ssize_t Put_Data(int FileD,int* Errono);
    
    //返回缓存区所有未读取的内容并初始化
    std::string Get_All_Data_String();
};