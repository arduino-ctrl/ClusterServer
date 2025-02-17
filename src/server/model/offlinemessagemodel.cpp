#include"offlinemessagemodel.hpp"
#include"db.h"

//存储用户的离线消息
void OfflineMsgModel::insert(int userid,string msg){
    //1. 组成sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d,'%s')",userid,msg.c_str());
    //2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

//删除用户的离线消息
void OfflineMsgModel::remove(int userid){
    //1. 组成sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid=%d",userid);
    //2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

//查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid){
    //1. 组成sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d",userid);
    //2.连接数据库
    vector<string> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res= mysql.query(sql);
        //res不为空，查询成功
        if(res!=nullptr){
            //把userid用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res))!=nullptr){
                vec.push_back(row[0]);
            }
            //res使用指针动态分配资源，需要释放，防止内存泄漏
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}
