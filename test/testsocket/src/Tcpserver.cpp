#include<iostream>
using namespace std;
#include"Tcpserver.h"
#include <poll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

Tcpserver::Tcpserver(string ip,int port):s_ip(ip),s_port(port){};


void Tcpserver::init_server_socket(){

    //初始化服务器套接字：IPV4,TCP字节流，默认协议
    listensock=socket(AF_INET,SOCK_STREAM,0);
    if (listensock < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    //绑定地址
    struct  sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    //此ip和port是用户指定的
    servaddr.sin_addr.s_addr=inet_addr(s_ip.c_str());
    servaddr.sin_port=htons(s_port);

    if (bind(listensock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind error");
        close(listensock);
        exit(EXIT_FAILURE);
    }
    //监听链接请求,后面的数字是监听的线程数
    if (listen(listensock, 5) < 0) {
        perror("listen error");
        close(listensock);
        exit(EXIT_FAILURE);
    }

    //把服务器监听描述符放入poll监听数组,监听的事件为可读事件，即是否有客户端连接
    fds.push_back({listensock,POLLIN,0});

}

void Tcpserver::accept_connection(){

    struct sockaddr_in cliaddr;
    socklen_t cliaddelen=sizeof(cliaddr);\
    //创建客户端服务器数据文件描述符，为后续数据传输做准备
    datasock=accept(listensock,(struct sockaddr*)&cliaddr,&cliaddelen);
    if (datasock >= 0) {
        fds.push_back({datasock, POLLIN, 0});
        cout << "Connect from " << inet_ntoa(cliaddr.sin_addr) << ":" << ntohs(cliaddr.sin_port) << endl;
        for(auto& it:fds){
            cout<<"pollfd.fd= "<<it.fd<<endl;
        }
    } else {
        perror("accept error");
    }

}

ssize_t Tcpserver::serrecv(int socket,string& show_inbuff){
    //定义写入的缓冲区
    char buff[1024];
    ssize_t nbytes=recv(socket,buff,sizeof(buff),0);
    //说明没有数据写入
    if (nbytes <= 0) {
        if (nbytes == 0) {
            cout << "Client disconnected" << endl;
        } else {
            perror("recv error");
        }
    } 
    else{
        show_inbuff.append(buff,nbytes);
    }
    return nbytes;
}


void Tcpserver::sersend(int socket,string& data){
    send(socket,data.data(),data.size(),0);
}


void Tcpserver::start(){

    init_server_socket();

    while(true){
        //循环监听poll监听结构体数组里面所有内容
        int nfds=poll(fds.data(),fds.size(),-1);
        //错误处理机制：如果没有服务器描述符存在
        if(nfds==-1){
            cout<<"没有服务器描述符存在"<<endl;
            return;
        }
        
        //处理就绪事件，包括接收连接，读写数据
         for (size_t i = 0; i < fds.size(); ++i) {
            //如果发生的事件是有数据可读，也就是连接或者客户端读取数据
            if (fds[i].revents & POLLIN) {
                //如果文件描述符是服务器的socket，说明是客户端的连接请求
                if (fds[i].fd == listensock) {
                    accept_connection();
                } 
                //说明是客户端数据写入缓冲区，服务器读取：recv
                else {
                    //服务器读取数据并显示
                    string inbuff;
                    ssize_t cnt = serrecv(fds[i].fd, inbuff);
                    if (cnt > 0) {
                        cout << "Received data: " << inbuff << endl;
                        fds[i].events |= POLLOUT;// 设置 POLLOUT 事件
                    } else if (cnt == 0) {
                        // 客户端关闭连接
                        close(fds[i].fd);
                        fds.erase(fds.begin() + i);
                        --i;
                    }
                }
            }
            //像客户端发送数据
            if (fds[i].revents & POLLOUT) {
                string outbuff = "Hello, Client!";
                sersend(fds[i].fd, outbuff);
                fds[i].events &= ~POLLOUT;// 清除 POLLOUT 事件
            }
        }
    }
    
}

