#include"chatserver.hpp"
#include"json.hpp"
#include<functional>
#include<string>
#include"chatservice.hpp"
using namespace std;
using namespace placeholders;
using json=nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg) : _server(loop, listenAddr, nameArg), _loop(loop)
{
    //注册连接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
    
    //注册读写回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

    //设置线程数量
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start();
}

// 上报连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    //客户端断开连接
    if (!conn->connected())
    {
        //客户端异常关闭
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf=buffer->retrieveAllAsString();
    //数据的反序列化
    json js=json::parse(buf);
    //实现网络模块和业务模块的解耦：通过hs["msgid"]调取对应业务handler（回调业务代码），可以将conn，js，time传给handler，执行回调
    //解耦操作一般两种：1.面向基类2.回调操作
    //js["msgid"].get<int>():利用get方法将json对象转换为int类型，get方法提供的是模板
    auto msgHanndler=ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHanndler(conn,js,time);
}
