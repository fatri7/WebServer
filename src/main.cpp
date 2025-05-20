#include <unistd.h>
#include <fstream>
#include "/home/fatri7/Coding/coding/My_Try/webserve/include/webserver.h"
#include "json.hpp"  // nlohmann/json 库

using json = nlohmann::json;
int main() {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 

    try {
        // 打开并解析配置文件
        std::ifstream config_file("config.json");
        if (!config_file.is_open()) {
            throw std::runtime_error("无法打开配置文件 config.json");
        }

        json config;
        config_file >> config;

        // 从JSON中读取配置参数，提供默认值
        int port = config.value("port", 1234);
        int trig_mode = config.value("trig_mode", 3);
        int timeout_ms = config.value("timeout_ms", 60000);
        bool opt_linger = config.value("opt_linger", false);
        int thread_number = config.value("thread_number", 4);
        bool daemon_mode = config.value("daemon_mode", false);

        // 如果需要以守护进程模式运行
        if (daemon_mode) {
            int result = daemon(1, 0);
            if (result != 0) {
                std::cerr << "Failed to daemonize: " << strerror(errno) << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        // 创建并启动服务器
        WebServe server(port, trig_mode, timeout_ms, opt_linger, thread_number);            
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}