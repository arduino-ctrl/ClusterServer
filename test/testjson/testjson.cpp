#include"json.hpp"
using json=nlohmann::json;

#include<iostream>
using namespace std;
#include<vector>
#include<map>
#include<string>

void test01(){
    //创建json对象
    json js;
    //json使用类似于unordered_map,键值对、无序
    //值类型可以是字符串、数组、也可以是json字符串（同一个键不同键值）
    js["id"]={1,2,3,4};
    js["msg"]["text1"]="aaa";
    js["msg"]["text1"]="bbb";
    js["abd"]={{"1","yyy"},{2,"zzz"}};
    js["msg_type"]=2;
    js["from"]="zhang san";
    js["to"]="li si";
    js["ask"]="hello";
    cout<<js<<endl;
    /*{"ask":"hello","from":"zhang san","msg_type":2,"to":"li si"}*/
    //在网络中需要数据序列化转化为字节流或者字符流
    string sendbuf=js.dump();
    //转换为c风格的字符串char*
    cout<<sendbuf.c_str()<<endl;
    /*{"ask":"hello","from":"zhang san","msg_type":2,"to":"li si"}*/
}
//json容器序列化
void test02(){
    json js;
    vector<int>v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    js["list"]=v;
    map<int,string>m;
    m.insert({1,"aaa"});
    m.insert({2,"bbb"});
    m.insert(make_pair(3,"ccc"));
    js["map"]=m;
    cout<<js<<endl;
    /*{"list":[1,2,3],"map":[[1,"aaa"],[2,"bbb"],[3,"ccc"]]}*/
}

//json数据反序列化
string test03(){
    //创建json对象
    json js;
    js["msg_type"]=2;
    js["from"]="zhang san";
    js["to"]="li si";
    js["ask"]="hello";
    cout<<js<<endl;
    /*{"ask":"hello","from":"zhang san","msg_type":2,"to":"li si"}*/
    //在网络中需要数据序列化转化为字节流或者字符流
    string sendbuf=js.dump();
    return sendbuf;
}
int main(){
    //test01();
    //test02();
    string recvBuf=test03();
    //进行反序列化，由字符串转化为数据对象，看作容器方便访问
    json jsBuf=json::parse(recvBuf);
    cout<<jsBuf["msg_type"]<<endl;
    cout<<jsBuf["from"]<<endl;
    cout<<jsBuf["to"]<<endl;
    cout<<jsBuf["ask"]<<endl;
    /*2
    "zhang san"
    "li si"
    "hello"
    */
    return 0;
}
