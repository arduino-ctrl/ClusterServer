#include <iostream>
#include <thread>
#include "TcpserverEpoll.h"
#include "Tcpclient.h"

void run_server() {
    // 创建一个监听IP地址为127.0.0.1，端口为8080，线程数为4的服务器
    Tcpserver server("127.0.0.1", 8080, 4);
    // 启动服务器
    server.start();
}

void run_client() {
    // 创建客户端，连接到127.0.0.1:8080
    Tcpclient client("127.0.0.1", 8080);

    if (client.connection()) {
        std::cout << "Connected to server" << std::endl;

        // 发送数据
        std::string message = "Hello, Server!";
        client.clisend(message);

        // 接收数据
        std::string response;
        client.clirecv(response);
        std::cout << "Received from server: " << response << std::endl;
    } else {
        std::cout << "Failed to connect to server" << std::endl;
    }
}

int main() {
    // 启动服务器线程
    std::thread server_thread(run_server);

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 启动客户端线程
    std::thread client_thread(run_client);

    // 等待线程完成
    server_thread.join();
    client_thread.join();

    return 0;
}
