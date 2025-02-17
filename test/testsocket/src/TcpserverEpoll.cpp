#include<iostream>
using namespace std;
#include "TcpserverEpoll.h"
#include<sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <sys/epoll.h>
#include <thread>

Tcpserver::Tcpserver(string ip,int port,int numThreads):s_ip(ip),s_port(port),m_numThreads(numThreads),events(1024){};

void Tcpserver::init_server_socket(){
    //初始化服务器套接字：IPV4,TCP字节流，默认协议
    listensock=socket(AF_INET,SOCK_STREAM,0);
    if (listensock < 0) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    //绑定地址
    struct  sockaddr_in servaddr;
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    //此ip和port是用户指定的
    servaddr.sin_addr.s_addr=inet_addr(s_ip.c_str());
    servaddr.sin_port=htons(s_port);

    if (bind(listensock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind error");
        close(listensock);
        exit(EXIT_FAILURE);
    }
    //监听链接请求,后面的数字是监听的线程数
    if (listen(listensock, m_numThreads) < 0) {
        perror("listen error");
        close(listensock);
        exit(EXIT_FAILURE);
    }

    cout<<"server listensock= "<<listensock<<endl;


}

//利用epoll_ctl方法将数据传输套接字添加到epoll实例中
void Tcpserver::accept_connection(){

    struct sockaddr_in cliaddr;
    socklen_t cliaddelen=sizeof(cliaddr);\
    //创建客户端服务器数据文件描述符，为后续数据传输做准备
    int datasock=accept(listensock,(struct sockaddr*)&cliaddr,&cliaddelen);

    cout<<"client datasock= "<<datasock<<endl;

    //利用多线程管理多个客户端与服务器的连接，采用hash表结构（key：datasock,value:ConnectionHandler）
    //使用 std::make_unique 创建一个 ConnectionHandler 对象的 std::unique_ptr，并将其存储在哈希表中。
    //这样，哈希表中的每个条目都管理一个客户端连接。
    m_handlers[datasock]=make_unique<ConnectionHandler>(datasock);

    //构建数据可读event，将数据连接套接字添加到epoll实例中。同时设置触发模式为边缘触发
    struct epoll_event event;
    event.data.fd=datasock;
    event.events=EPOLLIN|EPOLLET;

    epoll_ctl(epfd,EPOLL_CTL_ADD,datasock,&event);
}

//相当于生产者：锁定互斥锁，设置条件，通知等待的线程
void Tcpserver::distribute_events(vector<EventHandler*>&& handlers){
    //定义一个作用域块，控制锁的生命周期
    {
        //把就绪事件线程放入事件队列，工作线程关注事件队列
        unique_lock<mutex> lock(m_eventQueueMutex);//锁定互斥锁
        for (auto handler : handlers) {
            m_eventQueue.push(handler);//设置条件
        }
    }
    //条件变量通知等待的线程
    m_eventQueueCond.notify_all();
}

//相当于消费者：锁定互斥锁，检查条件是否满足，执行操作
void Tcpserver::work_thread(){
    //工作线程循环等待事件队列的通知
    while(true){
        EventHandler* handler = nullptr;
        //定义一个作用域块，控制锁的生命周期
        {
            /*
            这是一个条件变量等待操作。线程会在 m_eventQueueCond 上等待，直到队列 m_eventQueue 非空。
            Lambda 表达式 [this] { return !m_eventQueue.empty(); } 用于检查条件是否满足。[this] 表示捕获当前对象的 this 指针。
            */
            unique_lock<mutex> lock(m_eventQueueMutex);//锁定互斥锁，互斥锁在离开作用域后自动释放
            m_eventQueueCond.wait(lock, [this] { return !m_eventQueue.empty(); });//检查条件是否满足
            //执行操作
            handler = m_eventQueue.front();
            m_eventQueue.pop();
        }
        //从就绪事件队列中提取子线程，执行相应的操作（读写数据）
        if (handler) {
            handler->handleInput(handler->getsocket());
            handler->handleOutput(handler->getsocket());
        }

    }
}

void Tcpserver::start(){

    //初始化服务器套接字
    init_server_socket();

    //创建epoll实例

    epfd=epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create error");
        close(listensock);
        exit(EXIT_FAILURE);
    }

    //将监听套接字添加到epoll实例中
    struct epoll_event event;
    event.data.fd=listensock;
    event.events=EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listensock, &event) < 0) {
        perror("epoll_ctl error");
        close(listensock);
        close(epfd);
        exit(EXIT_FAILURE);
    }

    //启动工作线程
    vector<thread> threads;
    for (int i = 0; i < m_numThreads; ++i) {
        threads.emplace_back(&Tcpserver::work_thread, this);
    }

    //事件循环：建立连接，分发就绪事件给工作线程
    while(true){
        //利用epoll_wait()等待返回已发生的事件[从evlist里面获取已发生的事件]
        int nfds = epoll_wait(epfd, events.data(), events.size(), -1);
        if (nfds < 0) {
            perror("epoll_wait error");
            break;
        }
        //建立就绪事件线程数组
        vector<EventHandler*> handlers;
        //遍历就绪事件，把读写事件放入线程数组
        //【注意：主循环负责监听事件类型和把事件放到线程数组，之后并把线程数组分发到线程队列；在工作线程关注的是线程队列执行子线程的任务】
        for(int i=0;i<nfds;i++){
            //监听已发生事件文件描述符
            int sockfd=events[i].data.fd;
            //如果是监听描述符，说明要建立连接
            if(sockfd==listensock){
                accept_connection();
            }
            //说明要进行数据传输,需要监听具体是读还是写
            else{
                //服务器接收，根据哈希映射得到线程
                if(events[i].events&EPOLLIN){
                    cout<<"clitoserv "<<endl;
                    handlers.push_back(m_handlers[sockfd].get());
                }
                //服务器发送
                if(events[i].events&EPOLLOUT){
                    handlers.push_back(m_handlers[sockfd].get());
                }
            }
        }
        // 分发事件给工作线程
        distribute_events(std::move(handlers));
    }
    // 等待工作线程退出
    for (auto& thread : threads) {
        thread.join();
    }

}

// ssize_t Tcpserver::serrecv(int socket,string& show_inbuff){
//     //定义写入的缓冲区
//     char buff[1024];
//     ssize_t nbytes=recv(socket,buff,sizeof(buff),0);
//     //说明没有数据写入
//     if (nbytes <= 0) {
//         if (nbytes == 0) {
//             cout << "Client disconnected" << endl;
//         } else {
//             perror("recv error");
//         }
//     } 
//     else{
//         show_inbuff.append(buff,nbytes);
//     }
//     return nbytes;
// }

// void Tcpserver::sersend(int socket,string& data){
//     send(socket,data.data(),data.size(),0);
// }


/*-------------连接器类实现-----------*/
//服务器读取数据并处理
void ConnectionHandler::handleInput(int sockfd){

    //服务器读取数据并显示
    string inbuff;
    char buff[1024];
    ssize_t nbytes=recv(sockfd,buff,sizeof(buff),0);
     if (nbytes <= 0) {
        if (nbytes == 0) {
            cout << "Client disconnected" << endl;
        } else {
            perror("recv error");
        }
    } 
    else{
        inbuff.append(buff,nbytes);
        cout << "Received data: " << inbuff << endl;
    } 
}

//服务器输出数据并处理
void ConnectionHandler::handleOutput(int sockfd){
    string outbuff = "Hello, Client!";
    send(sockfd,outbuff.data(),outbuff.size(),0);
}

//获取服务器套接字
int ConnectionHandler::getsocket() {

    return m_sockfd;
}