/*
实现服务器类：采用IO复用模型，面向对象设计模式
1、初始化服务器套接字
2、绑定地址
3、监听连接请求
4、接收连接
5、读写数据
*/
#pragma once
#include<iostream>
#include<string>
#include<vector>
#include <poll.h>
using namespace std;

//服务器端采用poll()监视套接字活动

class Tcpserver{

public:
    //构造函数：服务器IP地址和端口号初始化
    Tcpserver(string ip,int port);
    
    //初始化套接字并绑定地址，监听连接请求
    void init_server_socket();

    //接收连接
    void accept_connection();

    //发送数据
    void sersend(int socket,string& data);

    //接收数据
    ssize_t serrecv(int socket,string& show_inbuff);

    //具体实现初始化套接字，绑定地址，监听链接请求，接收链接，读写数据
    void start();


private:
    //服务器IP地址
    string s_ip;
    //服务器端口号
    int s_port;
    //服务器socket监听套接字
    int listensock;
    //服务器数据传输套接字
    int datasock;
    //poll监视器结构体数组
    vector<struct pollfd> fds;
};
