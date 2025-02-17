
#include "Tcpserver.h"
#include "Tcpclient.h"
#include <thread>

void server_thread() {
    Tcpserver server("127.0.0.1", 8081);
    server.start();
}

void client_thread(int id) {
    Tcpclient client("127.0.0.1", 8081);
    if (client.connection()) {
        string message = "Hello, Server from client " + to_string(id) + "!";
        client.clisend(message);
        this_thread::sleep_for(chrono::milliseconds(100)); // 等待服务器处理
        string response;
        client.clirecv(response);
        cout << "Client " << id << " received from server: " << response << endl;
    }
}

int main() {
    // 创建并启动服务器线程
    thread server(server_thread);
    this_thread::sleep_for(chrono::seconds(1)); // 确保服务器先启动

    // 定义客户端数量为 10
    const int client_count = 10;
    // 创建一个存储客户端线程的向量
    vector<thread> clients;
    // 启动多个客户端线程
    /*
    在 clients 向量的末尾添加一个新的线程对象，并启动一个新的线程执行 client_thread 函数。
    client_thread 是一个函数，定义了客户端的连接和通信逻辑。
    i 作为参数传递给 client_thread 函数，表示客户端的编号。
    */
    for (int i = 0; i < client_count; ++i) {
        clients.emplace_back(client_thread, i);
    }

    // 等待所有客户端线程结束
    //auto& 表示自动推导类型并使用引用，避免拷贝线程对象。
    for (auto& client : clients) {
        client.join();
    }

    // 等待服务器线程结束
    server.join();

    return 0;
}


