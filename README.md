# ClusterServer
基于json+muduo+mysql+nginx+redis的集群服务器与客户端通信源码

1 技术栈

  ○ Json 序列化与反序列化
  
  ○ muduo网络库开发
  
  ○ nginx源码编译安装与环境部署
  
  ○ nginx的tcp负载均衡器配置
  
  ○ redis缓存服务器编程实践
  
  ○ 基于发布-订阅的服务器中间件redis消息队列编程实践
  
  ○ MySQL数据库编程
  
  ○ Cmake构建编译环境
  
  ○ Github托管项目

2 项目需求

2.1 客户端用户注册与登录

2.2添加好友和群组

2.3好友聊天与群组聊天

2.4 nginx配置tcp负载均衡

2.5 集群聊天系统支持客户端跨服务器通信

3 项目目标
3.1 掌握服务器的网络I/O模块、业务模块、数据模块分层设计的思想

3.2 掌握C++ muduo网络库的编程以及实现原理

3.3 掌握Json的编程应用

3.4 掌握nginx配置部署tcp负载均衡器的应用以及实现原理

3.5 掌握服务器中间件的应用场景以及基于发布-订阅的redis编程实践与应用原理

3.6 掌握Cmake构建自动化编译环境

4 项目编译方式

  cd build
  
  rm -rf *
  
  cmake ..
  
  make
  
5 项目运行方式

  cd bin
  
  1.服务器(nginx负载均衡)：./Chatserver 127.0.0.1 6000、./Chatserver 127.0.0.1 6002
  
  2.客户端：./Chatclient 127.0.0.1 8000
  
  
6 项目原理讲解：

https://blog.csdn.net/qq_54412097/category_12880233.html?fromshare=blogcolumn&sharetype=blogcolumn&sharerId=12880233&sharerefer=PC&sharesource=qq_54412097&sharefrom=from_link
