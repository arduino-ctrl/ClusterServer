#include<iostream>
using namespace std;
#include"Tcpclient.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>

Tcpclient:: Tcpclient(string ip,int port):c_ip(ip),c_port(port),clisock(-1){};

Tcpclient::~Tcpclient(){
    if(clisock!=-1){
        close(clisock);
    }
}

bool Tcpclient::connection(){

    //初始化客户端套接字：IPV4,TCP字节流，默认协议
    clisock=socket(AF_INET,SOCK_STREAM,0);

    //绑定地址
    struct  sockaddr_in cliaddr;
    memset(&cliaddr,0,sizeof(cliaddr));
    cliaddr.sin_family=AF_INET;
    //此ip和port是用户指定的
    cliaddr.sin_addr.s_addr=inet_addr(c_ip.c_str());;
    cliaddr.sin_port=htons(c_port);

    //连接服务器

    if(connect(clisock,(struct sockaddr*)&cliaddr,sizeof(cliaddr))==-1){
        return false;
    }
    else{
        return true;
    }

}

ssize_t Tcpclient::clisend(string& data){
    return send(clisock,data.data(),data.size(),0);
}

ssize_t Tcpclient::clirecv(string& buff){
    char temp[1024];
    ssize_t nbytes=recv(clisock,temp,sizeof(temp),0);
    //说明没有数据接收到
    if (nbytes <= 0) {
        if (nbytes == 0) {
            cout << "服务器关闭连接" << endl;
        } else {
            perror("recv error");
        }
    }
    else {
        buff.append(temp, nbytes);
    }
    return nbytes;
}

