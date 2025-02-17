/*
客户端主要实现功能如下：

1、连接服务器
2、发送数据
3、接收响应

*/

#pragma once

#include<iostream>
using namespace std;

class Tcpclient{


public:
    Tcpclient(string ip,int port);

    ~Tcpclient();
    //连接服务器
    bool connection();

    ssize_t clisend(string& data);

    ssize_t clirecv(string& buff);


private:

    //客户端ip
    string c_ip;
    //客户端端口号
    int c_port;
    //客户端socket
    int clisock;

};



