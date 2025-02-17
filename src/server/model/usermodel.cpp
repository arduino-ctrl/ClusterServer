#include"usermodel.hpp"
#include"db.h"
#include<iostream>
#include<muduo/base/Logging.h>
using namespace std;
//User表的增加方法
bool UserModel::insert(User &user)
{
    //1. 组成sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(username,password,state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());
    LOG_INFO<<sql;
    //2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        if(mysql.update(sql)){
            //获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

//根据用户号码查询用户信息
User UserModel::query(int id)
{
    //1. 组成sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d",id);
    //2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES* res= mysql.query(sql);
        //res不为空，查询成功
        if(res!=nullptr){
            //返回查到的行,得到的是字符串，可以用[ ]取值
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row!=nullptr){
                User user;
                //Convert a string to an integer.
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                //res使用指针动态分配资源，需要释放，防止内存泄漏
                mysql_free_result(res);
                return user;
            }
        }
    }
    //返回默认user，匿名对象
    return User();
}

//更新用户的状态信息
bool UserModel::updateState(User user)
{
    //1. 组成sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state='%s' where id = %d",
            user.getState().c_str(), user.getId());
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            return true;
        }
    }
    return false;
}

//重置用户状态信息
void UserModel::resetState()
{
    //1. 组成sql语句
    char sql[1024] = "update user set state='offline' where state = 'online'";
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}