#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include"user.hpp"
#include<vector>
using namespace std;
//维护好友信息的操作接口方法
class FriendModel
{
public:
    //添加好友关系
    void insert(int userid,int friendid);

    //返回用户好友列表（服务器发送给客户端，正常来说客户端直接保存好友列表，减轻服务器压力）
    //friendid 和 userid 做联合查询（联合主键），直接查询user表，返回friendid name state
    vector<User> query(int userid);

};

#endif