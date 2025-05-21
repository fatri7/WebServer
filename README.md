# webserver
https://github.com/agedcat/WebServer

# WebServe

WebServe 是一个基于 C++20、epoll 和线程池的高性能 HTTP 服务器，支持高并发连接管理、定时任务调度、任务提交和高效的 HTTP 请求/响应处理。项目结构清晰，适合学习和实际部署现代 Web 服务。

## 主要特性

- **高并发 HTTP 连接管理**：基于 epoll 的 IO 多路复用，支持边缘/水平触发。
- **线程池与协程支持**：任务调度高效，支持每线程最大协程数限制。
- **高效缓冲区管理**：自适应扩容，支持与文件描述符高效读写。
- **HTTP 协议完整支持**：请求解析、响应生成、静态文件映射、Keep-Alive。
- **定时器管理**：小根堆实现，支持连接超时自动关闭。
- **易于扩展与维护**：模块化设计，接口清晰。

## 目录结构

```
webserve/
├── src/
├── bin/
├── include/
├── README.md
└── ...
```

## 组件简介

### 1. Buffer

- 高效管理内存缓冲区，支持自动扩容、空间复用。
- 提供与文件描述符的高效数据交互接口。

### 2. Epoller

- 封装 epoll API，支持事件的添加、修改、删除与等待。
- 提供高效的 IO 多路复用能力。

### 3. HttpConnection

- 管理单个 HTTP 连接的生命周期。
- 负责连接初始化、关闭、读写缓冲区管理、请求解析与响应生成。

### 4. HttpRequest

- 解析 HTTP 请求行、头部和体，支持 GET/POST。
- 支持表单数据解析与 Keep-Alive 检测。

### 5. HttpResponse

- 生成标准 HTTP 响应，支持常见 MIME 类型和错误页面。
- 支持文件 mmap 映射，提升静态资源访问性能。

### 6. TimerManager

- 小根堆实现高效定时器队列，支持定时任务回调。
- 用于连接超时检测与自动关闭。

### 7. CoroutineThreadPool

- 基于 C++20 协程和线程池，支持每线程最大协程数限制。
- 提交任务返回 `std::future`，线程安全。

### 8. WebServe 主类

- 封装 epoll、定时器、线程池和 HTTP 连接管理。
- 提供统一的服务器启动、事件处理和资源管理接口。

## 快速开始

1. **编译环境**：需要支持 C++20 的编译器（如 g++ 11+）。
2. **依赖**：Linux epoll、C++ STL、json.hpp、concurrentqueue.h、部分系统调用。
3. **编译示例**：

    ```sh
    g++ -std=c++20 -pthread -o webserve src/*.cpp
    ```

4. **启动服务器**：

    ```bash
    cd bin/
    ./tiny_web_server_2025
    ```
5. **测试**：
   Apache HTTP Server非本地测试
   ```bash
    ab -n8000 -c8000 http://ip:port/local
   ```
| 指标               | agedcat | this | 差异        |
|--------------------|--------------|--------------|-------------|
| 总测试时间         | 77.862秒     | 70.059秒     | this快11%   |
| 每秒请求数(RPS)    | 128.43       | 142.74       | this高11%   |
| 平均请求时间       | 77,862ms     | 70,059ms     | this快11%   |
| 传输速率           | 405.99 KB/s  | 451.21 KB/s  | this高11%   |
