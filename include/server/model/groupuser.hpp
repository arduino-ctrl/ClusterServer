#ifndef GROUPUSER_H
#define GROUPUSER_H

//群组用户，相比user的信息多了一个role（角色）信息，直接继承user，复用user的其他信息
#include "user.hpp"
class GroupUser : public User
{
public:
    void setRole(string role){
        this->role = role;
    }
    string getRole(){
        return this->role;
    }
private:
    //角色
    string role;
};
#endif