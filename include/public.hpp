#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/
//枚举常量代表该枚举类型的变量可能取的值，编译系统为每个枚举常量指定一个整数值
//代表EnMsgType可能为LOGIN_MSG/REG_MSG常量，每个常量有一个整数值1，2。。。
enum EnMsgType
{
    LOGIN_MSG = 1,//登录消息
    LOGIN_MSG_ACK,//登录响应消息
    LOGINOUT_MSG,//注销消息
    REG_MSG,//注册消息
    REG_MSG_ACK,//注册响应消息
    ONE_CHAT_MSG,//点对点聊天消息
    ADD_FRIEND_MSG,//添加好友消息
    CREATE_GROUP_MSG,//创建群组消息
    ADD_GROUP_MSG,//加入群组消息
    GROUP_CHAT_MSG,//群聊消息

};
#endif  