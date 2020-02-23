#include "epoll_serve.h"
#include "sqlite3.h"
#include <signal.h>

sqlite3 *db;
//处理SIGPIPE信号，当发消息给客户fd，但客户fd已经关闭了
void handler(int signo)
{
    cout<<"Get a signal :SIGPIPE !"<<endl;
    cout<<"the reason is maybe the message has closed when writing to it"<<endl;
}

int main(int argc, char const *argv[])
{
    
    if(argc != 3)
    {
        printf("usage %s port threadnum\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);

    if(port == 0)
    {
        printf("port must be Integer\n");
        return -1;
    }

    int threadnum = atoi(argv[2]);

    if(threadnum > 20)
    {      
        printf("error:threadnum is too many!\n");
        return -1;

    }

    if(port == 0)
    {
        printf("threadnum must be Integer\n");
        return -1;
    }

    char *Error_Message;
    int error_remind = sqlite3_open("test_1.db",&db);

    if(error_remind != SQLITE_OK)
    {
        cout<<"Open database fail!."<<endl;
        return -1;
    }else {
        cout<<"Open database successfully!."<<endl;
    }
    //创建用户信息表
    string operation = "create table user_1(name varchar(10) PRIMARY KEY,password varchar(128),phone_number varchar(11))"; 
    error_remind = sqlite3_exec(db,operation.c_str(),0,0,&Error_Message);
    if(error_remind != SQLITE_OK){
        cout<<"table is ready."<<endl;
    }else {
        cout<<"Create data table successfully."<<endl;
    }
    //创建日志表
    string operation2 = "create table log(content varchar(256),logtime TIMESTAMP default (datetime('now','localtime')))";
    error_remind = sqlite3_exec(db,operation2.c_str(),0,0,&Error_Message);

    if(error_remind != SQLITE_OK)
    {
        cout<<"table is ready."<<endl;
    }else {
        cout<<"Create log table successfully."<<endl;
    }
    //启动服务器
    EpollServer *epoll = new EpollServer(port, threadnum);
    cout<<"threr is the thread_id that has been created:\n";
    epoll->init();
    epoll->epoll();
    return 0;
}
