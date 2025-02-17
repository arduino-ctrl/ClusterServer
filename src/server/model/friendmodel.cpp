#include"friendmodel.hpp"
#include"db.h"

//添加好友关系
void FriendModel::insert(int userid,int friendid)
{
    //1. 组成sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values(%d,%d)",userid,friendid);
    //2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

//返回用户好友列表（服务器发送给客户端，正常来说客户端直接保存好友列表，减轻服务器压力）
//friendid 和 userid 做联合查询（联合主键），直接查询user表，返回friendid name state
vector<User> FriendModel::query(int userid)
{
    //1. 组成sql语句
    char sql[1024] = {0};
    //多表的联合查询：在friend表中根据给定的userid查多个friendid,再去user表中查多个firendid的详细信息：id name state
    sprintf(sql, "select a.id,a.username,a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d",userid);
    //2.连接数据库
    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res= mysql.query(sql);
        //res不为空，查询成功
        if(res!=nullptr){
            //把userid用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr){
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            //res使用指针动态分配资源，需要释放，防止内存泄漏
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}