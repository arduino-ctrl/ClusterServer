/*
muduo网络库主要提供了两个类：

TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序

三个重要的链接库：
libmuduo_net、libmuduo_base、libpthread

muduo库底层就是epoll+线程池，其好处是：
可以将网络I/O代码和业务代码区分开，用户只需关注业务，网络的连接断开、读写事件的上报与监控交给muduo库

只暴露两个业务接口：
1.用户的连接与断开
2.用户的可读写事件

*/
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<functional>// #bind
#include<iostream>
#include<string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

/*
基于muduo网络库的服务器开发
1.组合TcpServer对象
2.创建Eventloop事件循环对象的指针
3.明确Tcpserver构造函数需要什么参数，输出Chatserver的构造函数
    a.Tcpserver 主要提供两个回调函数：setConnectionCallback与setMessageCallback
    b.在构造函数中定义回调函数，在类中定义具体回调的函数的实现
4.在当前服务类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5.设置合适的服务端线程数量，muduo库会自己分配I/O线程和work线程

*/
class ChatServer{
public:
    //构造函数对TcpServer进行初始化，TcpServer没有默认构造
    ChatServer(EventLoop *loop,//事件循环 reactor
               const InetAddress &listenAddr,//ip + port
               const string &nameArg)// 线程/server的名称
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        //给服务器注册用户连接与断开的回调函数,当监听到连接/断开时执行onConnection，具体如何监听到是网络库的事情，无需用户操心
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));//相当于this.onConnection(TcpConnectionPtr&)
        //给服务器注册用户读写事件的回调函数
        _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
        //设置服务器端的线程数量，muduo库会自适应处理连接线程和工作线程的分配；CPU核数一般等于线程数
        //1个I/O线程，3个work线程
        _server.setThreadNum(4);
    }
    
    //开启事件循环
    void start(){
        _server.start();
    }

private:

    //专门处理用户的连接创建和断开，相当于epoll中监听到listenfd后执行accept
    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected()){
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<"state: online"<<endl;
        }
        else{
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<"state: offline"<<endl;
            conn->shutdown();//close(fd)
        }
    }
    //专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn,//连接的共享指针
                   Buffer *buffer,//缓冲区，存放数据
                   Timestamp time)//接收到数据的时间信息
    {
        //echo服务器
        string buf=buffer->retrieveAllAsString();
        cout<<"recv: "<<buf<<"time: "<<time.toString()<<endl;
        conn->send(buf);

    }
    TcpServer _server;// #1
    EventLoop *_loop;// #2 看作epoll

};

int main(){

    EventLoop loop;//类似于创建epoll
    InetAddress addr("127.0.0.1",6000);//本地回环地址，服务器仅接受来自本机的连接。适合在开发阶段进行本地调试。
    ChatServer server(&loop,addr,"ChatServer");
    server.start();//epoll_ctl，listenfd加入epoll
    loop.loop();//类似于epoll_wait,以阻塞的方式等待新用户连接，已连接用户的读写事件等
    return 0;
}