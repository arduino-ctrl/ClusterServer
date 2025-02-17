#include<iostream>
#include<vector>
#include<thread>
#include<string>
#include<chrono>
#include<ctime>
#include<map>
#include<functional>
using namespace std;
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"user.hpp"
#include"group.hpp"
#include"public.hpp"
#include"json.hpp"
using json = nlohmann::json;

//定义全局变量

//记录当前系统登录的用户信息
User g_currentUser;
//记录当前系统登录用户的好友列表信息
vector<User> g_currentUserFriendList;
//记录当前系统登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
//控制主菜单页面的开关
bool isMainMenuRunning = false;

//定义全局方法
//显示当前登录用户的基本信息
void showCurrentUserData();
//接受线程(发送线程和接受线程必须分离)
void readTaskHandler(int clientfd);
//获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
//主聊天页面程序
void mainMenu(int clientfd);
int main(int argc,char **argv)//argc代表参数数量 argv代表参数数组
{
    if(argc < 3){
        cerr<<"command invalid! example: ./Chatclient 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    //通过解析命令行参数传递的ip 和 port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    //创建client端的socket
    int clientfd = socket(AF_INET,SOCK_STREAM,0);
    if(-1 == clientfd){
        cerr<<"socket create error"<<endl;
        exit(-1);
    }
    //创建服务器ip+port
    sockaddr_in server;
    memset(&server,0,sizeof(sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);
    //client与server进行连接
    if(-1 == connect(clientfd,(sockaddr*)&server,sizeof(sockaddr_in))){
        cerr<<"connect server error"<<endl;
        close(clientfd);
        exit(-1);
    }
    //main线程用于接收用户输入、负责发送数据
    while(true){
        //显示首页面菜单 登录、注册、退出
        cout<<"=================="<<endl;
        cout<<"1. login"<<endl;
        cout<<"2. register"<<endl;
        cout<<"3. quit"<<endl;
        cout<<"=================="<<endl;
        cout<<"choice:";
        int choice = 0;
        cin>>choice;
        cin.get();//读掉缓冲区残留的回车，在读取整数之后将后面的回车也读掉，防止下次读缓冲区时候残留

        switch (choice)
        {
            case 1: // login
            {
                int id = 0;
                char password[50]={0};//用getline可以读取一行字符串，string会残留回车
                cout<<"userid:"<<endl;;
                cin>>id;
                cin.get();//读掉缓冲区残留的回车
                cout<<"password:"<<endl;
                cin.getline(password,50);
                //json序列化
                json js;
                js["msgid"]=LOGIN_MSG;
                js["id"]=id;
                js["password"]=password;
                string request = js.dump();

                int len = send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
                if(len == -1){
                    cerr<<"send login msg error"<<request<<endl;
                }
                else{
                    //发送成功，阻塞等待发送消息的响应
                    char buffer[1024] = {0};
                    len = recv(clientfd,buffer,1024,0);
                    if(-1 == len){
                        cerr<<"recv login response error"<<endl;
                        
                    }
                    else{
                        //json反序列化
                        json responsejs = json::parse(buffer);
                        if(0!= responsejs["errno"].get<int>()){//登录失败
                            cerr<<responsejs["errmsg"]<<endl;
                        }
                        else{//登录成功
                            //记录当前用户的id 和 name
                            g_currentUser.setId(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"]);
                            //记录当前用户的好友列表信息
                            if(responsejs.contains("friends")){
                                //防止登录退出后重新登录导致信息重复
                                g_currentUserFriendList.clear();
                                vector<string> vec = responsejs["friends"];
                                for(string &str:vec){
                                    //json反序列化
                                    json js = json::parse(str);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }
                            //记录当前用户的群组列表信息
                            if(responsejs.contains("groups")){
                                //防止登录退出后重新登录导致信息重复
                                g_currentUserGroupList.clear();
                                vector<string> vec1 = responsejs["groups"];
                                for(string &str:vec1){
                                    //json反序列化
                                    json js = json::parse(str);
                                    Group group;
                                    group.setId(js["id"].get<int>());
                                    group.setName(js["groupname"]);
                                    group.setDesc(js["groupdesc"]);
                                    vector<string> vec2= js["users"];
                                    for(string &str:vec2){
                                        json js = json::parse(str);
                                        GroupUser groupuser;
                                        groupuser.setId(js["id"].get<int>());
                                        groupuser.setName(js["name"]);
                                        groupuser.setState(js["state"]);
                                        groupuser.setRole(js["role"]);
                                        group.getUsers().push_back(groupuser);
                                    }
                                    g_currentUserGroupList.push_back(group);
                                } 
                            }
                            //显示登录用户的基本信息
                            showCurrentUserData();
                            //显示当前用户的离线消息 个人聊天信息或者群组消息
                            if(responsejs.contains("offlinemsg")){
                                vector<string> vec = responsejs["offlinemsg"];
                                for(string &str:vec){
                                    json js = json::parse(str);
                                    //time + [id] +name +"said: "+ xxx
                                    int msgtype = js["msgid"].get<int>();
                                    if(ONE_CHAT_MSG==msgtype){
                                        cout << js["time"].get<string>() << "[ " << js["id"] << "]" << js["name"].get<string>()
                                             << "said: " << js["msg"].get<string>() << endl;
                                    }
                                    if(GROUP_CHAT_MSG==msgtype){
                                        cout <<"群消息["<<js["groupid"]<<"]"<< js["time"].get<string>() << "[ " << js["id"] << "]" << js["name"].get<string>()
                                             << "said: " << js["msg"].get<string>() << endl;
                                    }
                                }
                            }
                            //只需要开启一次接收线程，不能每次登录都开启一个，否则退出登录重新登录会重复开启接收线程
                            static int readThteadnum = 0;//static 关键字使变量在程序的整个生命周期内保持存在（即使退出当前 case 分支）数据段
                            if(readThteadnum==0){
                                //登录成功，启动接收线程负责接收数据
                                std::thread readTask(readTaskHandler,clientfd);//相当于调用pthread_create
                                //设置分离线程
                                readTask.detach();//相当于调用pthread_detach
                                readThteadnum++;
                            }
                            isMainMenuRunning = true;
                            //进入聊天主菜单页面，主线程
                            mainMenu(clientfd);
                        }
                    }
                }
            break;
            }
            case 2: // register
            {
                //json序列化
                json js;
                string username;
                string password;
                cout<<"username:"<<endl;
                cin>>username;
                cout<<"password:"<<endl;
                cin>>password;
                js["msgid"]=REG_MSG;
                js["name"] = username;
                js["password"] = password;
                string request = js.dump();

                int len = send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
                if(len == -1){
                    cerr<<"send reg msg error"<<request<<endl;
                }
                else{
                    //发送成功，阻塞等待发送消息的响应
                    char buffer[1024] = {0};
                    len = recv(clientfd,buffer,1024,0);
                    if(-1 == len){
                        cerr<<"recv reg response error"<<endl;
                        
                    }
                    else{
                        //json反序列化
                        json responsejs = json::parse(buffer);
                        if(0!= responsejs["errno"].get<int>()){//注册失败
                            cerr<<username<<"is already exist,register error!!"<<endl;
                        }
                        else{
                            //注册成功
                            cout<<username<<"register success,userid is"<<responsejs["id"]<<",do not forget it"<<endl;
                        }

                    }

                }
            break;
            }
            case 3: // quit
            {
                close(clientfd);
                exit(0);
            }
            default:
            {
                cerr<<"invalid input"<<endl;
                break;
            }
        }
    }
    return 0;
}
//接受线程(发送线程和接受线程必须分离)
void readTaskHandler(int clientfd)
{
    //个人聊天业务和群聊业务
    for(;;){
        char buffer[1024] = {0};
        int len = recv(clientfd,buffer,1024,0);//阻塞等待接收数据
        if(-1 == len||0 == len){
            close(clientfd);
            exit(-1);
        }
        //接收chatserver转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if(ONE_CHAT_MSG==msgtype){
            cout << js["time"].get<string>() << "[ " << js["id"] << "]" << js["name"].get<string>()
                 << "said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if(GROUP_CHAT_MSG==msgtype){
            cout <<"群消息["<<js["groupid"]<<"]"<< js["time"].get<string>() << "[ " << js["id"] << "]" << js["name"].get<string>()
                 << "said: " << js["msg"].get<string>() << endl;
            continue;
        }
    }

}
//显示当前登录用户的基本信息
void showCurrentUserData()
{
    cout<<"=====================login user====================="<<endl;
    cout<<"current login user => id:"<<g_currentUser.getId()<<"name: "<<g_currentUser.getName()<<endl;
    cout<<"---------------------friend list---------------------"<<endl;
    if(!g_currentUserFriendList.empty()){
        for(User &user:g_currentUserFriendList){
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    cout<<"---------------------group list---------------------"<<endl;
    if(!g_currentUserGroupList.empty()){
        for(Group &group:g_currentUserGroupList){
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser &groupuser:group.getUsers()){
                cout << groupuser.getId() << " " << groupuser.getName() << " "
                     << groupuser.getState() << " " << groupuser.getRole() << endl;
            }
        }
    }
    cout<<"===================================================="<<endl;
}
//获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date,"%d-%02d-%02d %02d:%02d:%02d",(int)ptm->tm_year+1900,(int)ptm->tm_mon+1,(int)ptm->tm_mday,(int)ptm->tm_hour,(int)ptm->tm_min,(int)ptm->tm_sec);
    return std::string(date);
}
//命令 handler
void help(int fd = 0,string str="");
void chat(int,string);
void addfriend(int,string);
void creategroup(int,string);
void addgroup(int,string);
void groupchat(int,string);
void loginout(int,string);
//系统支持的客户端命令列表
unordered_map<string,string> commandMap=
{
    {"help","显示所有支持的命令,格式help"},
    {"chat","一对一聊天,格式chat:friend:message"},
    {"addfriend","添加好友,格式addfriend:friendid"},
    {"creategroup","创建群组,格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组,格式addgroup:groupid"},
    {"groupchat","群聊,格式groupchat:groupid:message"},
    {"loginout","注销,格式loginout"}
};
//注册系统支持的客户端命令处理:map保存string和函数对象的映射，函数对象接收两个参数：int clientfd，string str
unordered_map<string,function<void(int,string)>> commandHandlerMap=
{
    //根据合法命令执行相应的方法即可
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"loginout",loginout}
};
//主聊天页面程序:1.显示提供的业务及命令 2.调用相应的业务
void mainMenu(int clientfd)
{
    help();
    char buffer[1024]={0};
    while(isMainMenuRunning){
        //获取命令
        cin.getline(buffer,1024);
        //命令转换为string
        string commandbuf(buffer);
        string command;//存储命令
        int idx = commandbuf.find(":");//提取冒号之前的命令
        if(-1 == idx){
            //针对help 和 loginout命令
            command = commandbuf;
        }
        else{
            //提取冒号之前的命令
            command = commandbuf.substr(0,idx);
        }
        //根据命令调用相应的业务操作
        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end()){
            cerr<<"invalid input command!!"<<endl;
            continue;
        }
        //调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx));//把冒号后面的字符串传入

    }

}
//命令 handler
void help(int fd,string str){
    cout<<"show command list>>>"<<endl;
    //遍历commandMap
    for(auto &s:commandMap){
        cout<<s.first<<": "<<s.second<<endl;
    }
    cout<<endl;
}
void chat(int clientfd,string str){
    int idx =str.find(":");
    if(-1 == idx){
        cerr<<"chat command invalid!"<<endl;
        return;
    }
    int toid = atoi(str.substr(0,idx).c_str());
    string message = str.substr(idx+1,str.size()-idx);
    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["to"]=toid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    //json序列化
    string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len){
        cerr<<"send chat msg error ->"<<buffer<<endl;
    }

}
void addfriend(int clientfd,string str){
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    //json序列化
    string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len){
        cerr<<"send addfriend msg error ->"<<buffer<<endl;
    }
}
void creategroup(int clientfd,string str)
{
    int idx =str.find(":");
    if(-1 == idx){
        cerr<<"creategroup command invalid!"<<endl;
        return;
    }
    string groupname = str.substr(0,idx);
    string groupdesc = str.substr(idx+1,str.size()-idx);
    int id = g_currentUser.getId();
    json js;
    js["msgid"]=CREATE_GROUP_MSG;
    js["id"]=id;
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;
    //json序列化
    string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0); 
    if(-1 == len){
        cerr<<"send creategroup msg error ->"<<buffer<<endl;
    }
}
void addgroup(int clientfd,string str)
{
    int id = g_currentUser.getId();
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=id;
    js["groupid"]=groupid;
    //json序列化
    string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len){
        cerr<<"send addgroup msg error ->"<<buffer<<endl;
    }
}
void groupchat(int clientfd,string str)
{
    int idx =str.find(":");
    if(-1 == idx){
        cerr<<"groupchat command invalid!"<<endl;
        return;
    }
    int groupid = atoi(str.substr(0,idx).c_str());
    string message = str.substr(idx+1,str.size()-idx);
    int id = g_currentUser.getId();
    json js;
    js["msgid"]=GROUP_CHAT_MSG;
    js["id"]=id;
    js["name"]=g_currentUser.getName();
    js["groupid"]=groupid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    //json序列化
    string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len){
        cerr<<"send groupchat msg error ->"<<buffer<<endl;
    }

}
void loginout(int clientfd,string)
{
    json js;
    js["msgid"]=LOGINOUT_MSG;
    js["id"]=g_currentUser.getId();
    //json序列化
    string buffer = js.dump();
    int len = send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1 == len){
        cerr<<"send loginout msg error ->"<<buffer<<endl;
    }
    isMainMenuRunning = false;
}
