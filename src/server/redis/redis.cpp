#include"redis.hpp"
#include<iostream>
#include<thread>
using namespace std;

//构造函数，将订阅和发布的两个对象指针置空
Redis::Redis():_publish_context(nullptr),_subcribe_context(nullptr)
{

}
//析构函数，释放资源
Redis::~Redis(){
    if(_publish_context!=nullptr){
        redisFree(_publish_context);
    }
    if(_subcribe_context!=nullptr){
        redisFree(_subcribe_context);
    }
}
//连接redis服务器
bool Redis::connect(){
    //两个对象连接redis服务器，redis默认ip+port=127.0.0.1：6379
    _publish_context = redisConnect("127.0.0.1", 6379);
    if(nullptr == _publish_context){
        cerr<<"connect redis failed!"<<endl;
        return false;
    }
    _subcribe_context = redisConnect("127.0.0.1", 6379);
    if(nullptr == _subcribe_context){
        cerr<<"connect redis failed!"<<endl;
        return false;
    }
    //在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    //因为上报和订阅都是阻塞的，需要单独开辟一个线程进行消息上报，否则服务器无法进行其他业务
    thread t([&](){
        observer_channel_message();
    });
    t.detach();
    cout<<"connect redis-server success!"<<endl;
    return true;
}

//向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message){
    //相当于命令行 publish channel message
    //redisCommand是同步操作，阻塞，相当于redisAppendCommand+redisBufferWrite+redisGetReply,pubilsh是一致性马上回复，所以可以阻塞等待
    //redisAppendCommand是将命令组装好后放到本地缓存
    //redisBufferWrite是将本地缓存的命令发送到redis服务器
    //redisGetReply是从redis服务器获取返回的结果（阻塞型）
    redisReply* reply = (redisReply*)redisCommand(this->_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if(nullptr == reply){
        cerr<<"publish message failed!"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}
//向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel){
    //subscribe命令本身会造成线程阻塞等待通道里发生消息，这里之作订阅通道，不接受通道消息
    //通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    //只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if(REDIS_ERR == redisAppendCommand(this->_subcribe_context, "SUBSCRIBE %d", channel)){
        cerr<<"subscribe channel failed!"<<endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while(!done){
        if(REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done)){
            cerr<<"subscribe channel failed!"<<endl;
            return false;
        }
    }
    return true;
}

//向redis指定的通道unsubscribe取消订阅消息（用户下线，无需订阅）
bool  Redis::unsubscribe(int channel){
    if(REDIS_ERR == redisAppendCommand(this->_subcribe_context, "UNSUBSCRIBE %d", channel)){
        cerr<<"subscribe channel failed!"<<endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while(!done){
        if(REDIS_ERR == redisBufferWrite(this->_subcribe_context, &done)){
            cerr<<"subscribe channel failed!"<<endl;
            return false;
        }
    }
    return true;
}

//在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message(){
    redisReply* reply = nullptr;
    //以循环阻塞的方式等待通道发生的消息
    while(REDIS_OK == redisGetReply(this->_subcribe_context, (void**)&reply)){
        //订阅收到的消息是一个redisReply对象，根据不同的消息类型进行处理
        //订阅收到的消息是一个带三元素的数组
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr){
            //回调通知上层应用，收到订阅的消息
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        //释放redisReply对象
        freeReplyObject(reply);
    }
    cerr<<"exit observer_channel_message"<<endl;
}

//初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}

/*

同步redis发布-订阅封装的代码主要提供以下方法：
1.connect：连接redis服务器生成了两个redisContext对象，一个用于发布消息（_publish_context），一个用于订阅通道(_subcribe_context)。
2.publish：在_publish_context上发布消息，id+msg，阻塞接收redis server的响应。
3.subscribe：在_subcribe_context上订阅通道，id，不接收redis server的响应。
4.unsubscribe：在_subcribe_context上取消订阅通道，id，不接收redis server的响应。
5.由于订阅/取消订阅接收响应都是阻塞型的，所以单独开辟线程thread：
    通过observer_channel_message函数调用redisGetReply循环阻塞方式接收订阅通道中的消息，回调通知上层应用（id+msg)。

*/