/*
利用epoll机制代替poll机制，无需poll机制下每次调用时复制整个文件描述符集合，只需要关注实际发生的事件

采用reactor模式：将主循环和事件处理逻辑分离开，主循环负责等待就绪事件分发给工作线程，工作线程利用线程池并行处理实际发生的事件
服务器类：
1、套接字初始化和连接
2、数据读写

定义以下结构：
1、Tcpserver类：套接字初始化、连接、事件分发
2、事件处理接口
2、连接事件处理器

*/

#pragma once
#include<iostream>
using namespace std;
#include <sys/epoll.h>
#include<vector>
#include<functional>
#include <condition_variable>
#include<queue>

//Reactor模式中的事件处理器接口
class EventHandler{
public:
    virtual ~EventHandler() {}
    virtual void handleInput(int sockfd) = 0;
    virtual void handleOutput(int sockfd) = 0;
    virtual int getsocket() = 0;
};

//Tcpserver主类
class Tcpserver{
public:
    //构造函数，初始化线程数、服务器ip和端口号
    Tcpserver(string ip,int port,int numThreads);

    //初始化套接字并绑定地址，监听连接请求
    void init_server_socket();

    //接收连接
    void accept_connection();

    //具体实现初始化套接字和epoll实例，绑定地址，监听链接请求，接收链接，读写数据
    void start();

    //工作线程
    void work_thread();

    //分发事件给工作线程,负责把就绪事件的线程数组分发给工作线程，实现多线程处理
    void distribute_events(vector<EventHandler*>&& handlers);

    // //发送数据
    // void sersend(int socket,string& data);

    // //接收数据
    // ssize_t serrecv(int socket,string& show_inbuff);


private:
    string s_ip;
    int s_port;
    int listensock;
    //线程数量
    int m_numThreads;
    //epoll实例
    int epfd;
    //结构体数组evlist，用于保存实际发生的事件信息
    vector<struct epoll_event> events;

    //多线程哈希表,用于保存多个客户端和服务器的连接与IO操作信息
    //智能指针：unique_ptr来管理EventHandler实例，避免内存泄露。【智能指针的用法与原理还需要复习！！！】
    //用于将每个客户端的套接字（datasock）映射到一个 std::unique_ptr，该指针指向一个 EventHandler 对象,
    //用父类进行定义，可以使得在hash表中保存不同类型的子类对象，实现多态
    unordered_map<int,unique_ptr<EventHandler>>m_handlers;

    //多线程与互斥锁相关变量
    //事件队列，用于后续分发给工作线程
    queue<EventHandler*> m_eventQueue;
    //定义事件互斥锁，用于保护共享资源，防止多个线程同时访问该资源导致的数据竞争和不一致性
    //过互斥锁，确保在同一时刻只有一个线程可以访问共享资源
    mutex m_eventQueueMutex;
    //条件变量用于线程间的通信，允许一个线程等待另一个线程发出的信号。
    condition_variable m_eventQueueCond;
    
};

//连接处理器：连接、数据读写
class ConnectionHandler:public EventHandler{
public:

    //连接器初始化,用于客户端与服务器连接的套接字传递
    ConnectionHandler(int socket):m_sockfd(socket){};

    //服务器读取数据并处理
    void handleInput(int sockfd);

    //服务器输出数据并处理
    void handleOutput(int sockfd);

    //获取服务器套接字
    int getsocket();
private:

    int m_sockfd;

};

