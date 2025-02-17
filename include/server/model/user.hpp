#ifndef USER_H
#define USER_H

#include<string>
using namespace std;
/*
mysql> SHOW COLUMNS FROM user;
+----------+--------------------------+------+-----+---------+----------------+
| Field    | Type                     | Null | Key | Default | Extra          |
+----------+--------------------------+------+-----+---------+----------------+
| id       | int                      | NO   | PRI | NULL    | auto_increment |
| username | varchar(50)              | NO   | UNI | NULL    |                |
| password | varchar(50)              | NO   |     | NULL    |                |
| state    | enum('online','offline') | YES  |     | offline |                |
+----------+--------------------------+------+-----+---------+----------------+
*/
//定义数据库对象，将数据库信息整合为一个对象提交给业务层
//匹配User表的ORM类
class User{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->password = pwd;
        this->state = state;
    }

    void setId(int id)
    {
        this->id = id;
    }

    void setName(string name)
    {
        this->name = name;
    }

    void setPwd(string pwd)
    {
        this->password = pwd;
    }

    void setState(string state)
    {
        this->state = state;
    }

    int getId()
    {
        return this->id;
    }

    string getName()
    {
        return this->name;
    }

    string getPwd()
    {
        return this->password;
    }

    string getState()
    {
        return this->state;
    }
protected:
    int id;
    string name;
    string password;
    string state;
};
#endif