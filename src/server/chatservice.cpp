#include"chatservice.hpp"
#include"public.hpp"

//利用muduo库的封装好的日志输出
#include<muduo/base/Logging.h>
#include<vector>
using namespace muduo;
using namespace std;

//获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}
// 注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});

    //连接redis服务器
    if(_redis.connect()){
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
}

MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid: " << msgid << "can not find handler";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登录业务 id+pwd 验证pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO<<"DO LOGIN SERVICE!";
    int id=js["id"];
    string pwd=js["password"];

    //查询id对应的user对象
    User user = _userModel.query(id);
    if(user.getId()==id&&user.getPwd()==pwd){
        if(user.getState()=="online"){
            //该用户已经登录，不允许重复登录
            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            //响应error number如果为0，表示业务成功
            response["errno"] = 2;
            response["errmsg"] = "该账号已经登录，请重新输入新账号";
            //组装好json通过网络返回给客户端
            conn->send(response.dump());//数据序列化
        }
        else{
            //登录成功，记录用户连接信息
            /*群组聊天时，onMessage会被多线程调用，同时这个记录用户连接的map也会被多线程调用
            并且这个map会不断发生变化，需要考虑 线程安全 的问题*/
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id,conn});
            }

            //id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            //登录成功，更新用户state->online
            user.setState("online");
            _userModel.updateState(user);
            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            //响应error number如果为0，表示业务成功
            response["errno"] = 0;
            response["id"]=user.getId();
            response["name"]=user.getName();

            //查询该用户是否有离线消息，如果有，则服务器返回给用户查看
            vector<string> vec = _offlineMsgModel.query(user.getId());
            if(!vec.empty()){
                //服务器响应离线消息
                response["offlinemsg"] = vec;
                //读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(user.getId());
            }

            //查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty()){
                vector<string> vec2;
                for(User &user:userVec){
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            //查询该用户的群组信息并返回
            vector<Group> groupVec = _groupModel.queryGroups(id);
            if(!groupVec.empty()){
                vector<string> vec3;
                for(Group &group:groupVec){
                    json groupjs;
                    groupjs["id"] = group.getId();
                    groupjs["groupname"] = group.getName();
                    groupjs["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for(GroupUser &user:group.getUsers()){
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    groupjs["users"] = userV;
                    vec3.push_back(groupjs.dump());
                }
                response["groups"] = vec3;
            }


            //组装好json通过网络返回给客户端
            conn->send(response.dump());//数据序列化
        }

    }
    else{
        //用户不存在或者用户存在密码错误，登录失败
        json response;
        response["msgid"]=LOGIN_MSG_ACK;
        //响应error number如果为0，表示业务成功
        response["errno"] = 1;
        response["errmsg"] = "用户名或者密码错误";
        //组装好json通过网络返回给客户端
        conn->send(response.dump());//数据序列化
    }
}
// 处理注册业务 
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "DO REGISTER SERVICE!";
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功,返回客户端json消息，并把自增的id返回给用户当作账号
        json response;
        response["msgid"]=REG_MSG_ACK;
        //响应error number如果为0，表示业务成功
        response["errno"] = 0;
        response["id"]=user.getId();

        //组装好json通过网络返回给客户端
        conn->send(response.dump());//数据序列化
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"]=REG_MSG_ACK;
        //响应error number如果为1，表示业务不成功
        response["errno"] = 1;
        //组装好json通过网络返回给客户端
        conn->send(response.dump());//数据序列化
    }
}

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    //做两件事：1.用户数据库state->offline 2._userConnMap中用户信息删除
    {
        lock_guard<mutex> lock(_connMutex);
        //遍历_userConnMap
        for(auto it=_userConnMap.begin();it!=_userConnMap.end();++it){
            if(it->second==conn){
                user.setId(it->first);
                //从map表删除用户的连接信息
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //用户下线，取消订阅redis channel
    _redis.unsubscribe(user.getId());

    //更新用户的状态信息
    if(user.getId()!=-1){
        user.setState("offline");
        _userModel.updateState(user);
    }
}

//服务器异常，业务重置方法
void ChatService::reset()
{
    //服务器异常断开，把online状态的用户设置成offline
    _userModel.resetState();
}

//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    //从json消息获取对方id
    int toid = js["to"].get<int>();
    //先查询本服务器上是否存在toid的用户
    //查看是否在线，线程安全，同时需要保证我在转发消息时候connection不被移除（加锁）
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it!=_userConnMap.end()){
            //toid在线,转发消息,服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    //如果本服务器没找到toid用户，toid要不在其他服务器，要不不在线
    //查询toid是否在线
    User user = _userModel.query(toid);
    if(user.getState()=="online"){
        //toid在线，但是不在本服务器上，转发消息到redis消息队列
        _redis.publish(toid,js.dump());
        return;
    }

    //toid不在线，存储离线消息到数据库offlinemessage
    _offlineMsgModel.insert(toid,js.dump());
}

//添加好友业务 msgid id firendid
void ChatService::addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    //当前用户id
    int userid = js["id"].get<int>();
    //好友id
    int friendid = js["friendid"].get<int>();

    //存储好友信息
    _friendModel.insert(userid,friendid);

}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];
    //存储新创建的群组信息
    Group group(-1,name,desc);
    if(_groupModel.createGroup(group)){
        //存储群组创建人信息
        _groupModel.addGroup(userid,group.getId(),"creator");
    }

}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    //获取群组中所有其他用户id
    vector<int> useridVec = _groupModel.queryGroupUsers(userid,groupid);
    //加锁，防止_userConnMap中的用户在发送消息时候上线或者下线，C++中map的操作本身是无法保证线程安全的
    lock_guard<mutex> lock(_connMutex);
    for(int id:useridVec){
        auto it = _userConnMap.find(id);
        //用户在线，并且在本台服务器，直接转发消息
        if(it!=_userConnMap.end()){
            it->second->send(js.dump());
        }
        //用户在其他服务器上或者不在线
        else{
            //查询其他用户是否在线,查询数据库
            User user = _userModel.query(id);
            if(user.getState()=="online"){
                //用户在线，但是不在本服务器上，转发消息到redis消息队列
                _redis.publish(id,js.dump());
            }
            else{
                //存储离线消息
                _offlineMsgModel.insert(id,js.dump());
            }

        }
    }
}

//处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn,json &js,Timestamp time)
{
    User user;
    int userid = js["id"].get<int>();
    user.setId(userid);
    //做两件事：1.用户数据库state->offline 2._userConnMap中用户信息删除
    {
        lock_guard<mutex> lock(_connMutex);
        //遍历_userConnMap
        for(auto it=_userConnMap.begin();it!=_userConnMap.end();++it){
            if(it->first==userid){
                //从map表删除用户的连接信息
                _userConnMap.erase(it);
                break;
            }
        }
    }

    //用户下线，取消订阅redis channel
    _redis.unsubscribe(userid);

    //更新用户的状态信息
    if(user.getId()!=-1){
        user.setState("offline");
        _userModel.updateState(user);
    }
}

//从redis消息队列中获取订阅的消息,将msg转发给对应的userid
void ChatService::handleRedisSubscribeMessage(int userid,string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if(it!=_userConnMap.end()){
        it->second->send(msg);
        return;
    }
    //存储离线消息，这里主要考虑在上报和调用回调过程中用户突然下线的情况
    _offlineMsgModel.insert(userid,msg);
}