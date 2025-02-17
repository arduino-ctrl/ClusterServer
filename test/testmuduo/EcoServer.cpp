/*利用Muduo库实现一个简单的Eco服务器*/
/*
头文件包含：
muduo/net
muduo/base

1、创建main Eventloop，循环事件监听器，负责循环监听新用户的连接情况
2、设置监听的地址和端口
3、创建sub Eventloop，封装为EchoServer类，包含Tcpserver和loop
4、向TcpServer注册各类事件的用户自定义的处理函数: setMessageCallback()、setConnectionCallback();并在private进行实现
5、创建EchoServer 对象，启动TcpServer
6、main Eventloop 执行loop（）；


工作流程：

主线程的EventLoop：
    创建并运行主EventLoop。
    监听新连接。

线程池中的EventLoop：
    TcpServer根据设置的线程数创建多个线程，每个线程运行一个EventLoop。
    当有新连接到来时，主EventLoop会将连接分配给线程池中的某个EventLoop。

处理连接：
    分配到线程池中的EventLoop会处理该连接的所有I/O事件（如读写数据）。

*/

#include<iostream>
using namespace std;
#include<muduo/base/Logging.h>
#include<muduo/net/EventLoop.h>
#include<muduo/net/TcpServer.h>
using namespace muduo;
using namespace muduo::net;
#include<functional>

// #include<sting>

class EchoServer{

public:

    //构造函数
    EchoServer(EventLoop *loop,
               const InetAddress &addr,
               const string &name)
        : server_(loop, addr, name),
          loop_(loop)
    {
        //设置用户处理函数的回调函数，当服务器检测到新连接并调用回调函数时，会将参数传递给成员函数onConnection并调用
        //利用bind创建一个绑定对象作为回调函数的参数，
        //第一个参数是成员函数的指针，也就是要回调的函数，第二个参数为当前类的实例对象，也就是调用哪个对象的成员函数
        //第三个参数为占位符，表示将来调用回调函数时提供第一个参数，而不是在绑定时提供。换句话讲，当调用回调函数时提供TcpConnectionPtr &conn，进而作为onConnection的参数，而不是绑定时提供
        //下面这个类似于当调用回调函数时，执行this->onConnection(conn)
        server_.setConnectionCallback(bind(&EchoServer::onConnection,this,placeholders::_1));

        //将用户定义的可读事件处理函数注册进TcpServer中，TcpServer发生可读事件时会执行onMessage函数。
        //占位符的简单使用：回调的函数有几个参数就写几个占位符
        server_.setMessageCallback(bind(&EchoServer::onMessage, this,
                                        placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        //设置sub reactor线程数，有几个线程就有几个subreactor，每个线程有一个独立的EventLoop。
        server_.setThreadNum(3);
    }

    void strat(){
        server_.start();
    }

private:

    //用户自定义的处理函数
    //用户连接处理函数，当服务端接收到新连接建立请求，则打印Connection 对象的地址和端口，如果是关闭连接请求，则打印Connection Down
    //TcpConnectionPtr是一个共享所有权的智能指针，可以在多个地方共享同一个 TcpConnection 对象的所有权
    // TcpConnection 是一个类或结构体，表示一个TCP连接。它封装了与TCP连接相关的操作和状态，例如连接的建立、断开、数据传输等
    void onConnection(const TcpConnectionPtr &conn){
        if (conn->connected())
        {
            //使用 LOG_INFO 宏记录一条信息，内容是连接的对端地址和端口
            LOG_INFO << "Connection from " << conn->peerAddress().toIpPort();
        }
        else
        {
            LOG_INFO << "Connection disconnected " << conn->peerAddress().toIpPort();
        }
    }
    //用户可读事件处理函数:当一个TCP发生了可读事件，就把接收到的消息原封不动传回去

    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp time)
    {
        //从缓冲区检索所有接收到的数据
        string msg=buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown();

    }

    //创建服务器对象
    TcpServer server_;
    //创建sub EventLoop
    EventLoop* loop_;
};

int main(){
    //定义main eventloop
    EventLoop loop;
    //封装socketaddr_in,设置监听地址和端口号，端口号为2007，ip地址为默认的0.0.0.0（INADDR_ANY，即服务器可以连接所有可用的网络接口）
    InetAddress addr(2007);

    EchoServer server(&loop,addr,"echoserver-01");

    server.strat();

    loop.loop();

    return 0;
}