#include"chatserver.hpp"
#include"chatservice.hpp"
#include<iostream>
#include<signal.h>
using namespace std;

//处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    //调用 exit 函数以退出程序。参数 0 表示程序正常退出
    exit(0);
}

int main(int argc,char **argv){

    //SIGINT 是一个预定义的信号常量，表示“中断信号”，通常由用户在终端按 Ctrl+C 产生。
    signal(SIGINT,resetHandler);

    if(argc < 3){
        cerr<<"command invalid! example: ./Chatserver 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    //通过解析命令行参数传递的ip 和 port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"chatserver");

    server.start();

    loop.loop();
    
    return 0;
}