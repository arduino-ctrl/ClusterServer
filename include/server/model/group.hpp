#ifndef GROUP_H
#define GROUP_H
#include <string>
#include <vector>
#include "groupuser.hpp"
using namespace std;

class Group{
public:
    Group(int id = -1, string name = "", string desc = ""){
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    void setId(int id){
        this->id = id;
    }
    void setName(string name){
        this->name = name;
    }
    void setDesc(string desc){
        this->desc = desc;
    }
    int getId(){
        return this->id;
    }
    string getName(){
        return this->name;
    }
    string getDesc(){
        return this->desc;
    }
    vector<GroupUser>& getUsers(){
        return this->users;
    }

private:
    //群组id
    int id;
    //群组名称
    string name;
    //群组描述
    string desc;
    //获取组内成员
    vector<GroupUser> users;

};

#endif