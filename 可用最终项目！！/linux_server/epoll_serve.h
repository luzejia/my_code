#ifndef _EPOLL_SERVER_H_
#define _EPOLL_SERVER_H_

#include "sqlite3.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/epoll.h>
#include "thread_pool.h"
#include <string>
#include <atomic>
#include "base64.h"
#include "time_heap.h"

#define TIMESLOT 10 
#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define TIMESLOT 10 
#define MAX_EVENT 1024
#define MAX_BUFFER 2048
#define shift(x, n) (((x) << (n)) | ((x) >> (32-(n))))
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))
#define A 0x67452301
#define B 0xefcdab89
#define C 0x98badcfe
#define D 0x10325476


extern sqlite3 * db;
time_heap client_time_heap(1024);
using namespace std;
atomic_int new_increace[1024];

client_data *users = new client_data[FD_LIMIT];
unsigned int strlength;
unsigned int atemp;
unsigned int btemp;
unsigned int ctemp;
unsigned int dtemp;
const unsigned int k[] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,
    0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,0x698098d8,
    0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,
    0xa679438e,0x49b40821,0xf61e2562,0xc040b340,0x265e5a51,
    0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,
    0xfcefa3f8,0x676f02d9,0x8d2a4c8a,0xfffa3942,0x8771f681,
    0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,
    0xbebfbc70,0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,
    0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,0xf4292244,
    0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,
    0xffeff47d,0x85845dd1,0x6fa87e4f,0xfe2ce6e0,0xa3014314,
    0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391 };

const unsigned int s[] = {7,12,17,22,7,12,17,22,7,12,17,22,7,
    12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,
    15,21,6,10,15,21,6,10,15,21,6,10,15,21 };

const char str16[] = "0123456789abcdef";

void mainLoop(unsigned int M[])
{
    unsigned int f,g;
    unsigned int a = atemp;
    unsigned int b = btemp;
    unsigned int c = ctemp;
    unsigned int d = dtemp;

    for (unsigned int i = 0; i < 64; i++)
    {
        if(i<16){
            f = F(b,c,d);
            g = i;
        }else if (i<32)
        {
            f = G(b,c,d);
            g = (5 * i + 1) % 16;
        }else if(i < 48){
            f = H(b, c, d);
            g = (3 * i + 5) % 16;
        }else{
            f = I(b, c, d);
            g = (7 * i) % 16;
        }

        unsigned int tmp = d;
        d = c;
        c = b;
        b = b + shift((a + f + k[i] + M[g]), s[i]);
        a = tmp;
    }

    atemp = a + atemp;
    btemp = b + btemp;
    ctemp = c + ctemp;
    dtemp = d + dtemp;
}

unsigned int * add(string str)
{
    unsigned int num = ((str.length() + 8) / 64) + 1;
    unsigned int *strByte = new unsigned int[num * 16];
    strlength = num * 16;
    for (unsigned int i = 0; i < num * 16; i++)
    {
        strByte[i] = 0;
    }

    for (unsigned int i = 0; i <str.length(); i++)
    {
        strByte[i >> 2]|= (str[i]) << ((i % 4) * 8);
    }

    strByte[str.length() >> 2]|= 0x80 << (((str.length() % 4)) * 8);
    strByte[num * 16 - 2] = str.length() * 8;
    return strByte;
}

string changeHex(int a)
{
    int b;
    string str1;
    string str = "";
    for(int i = 0;i < 4;i++)
    {
        str1 = "";
        b = ((a >> i * 8) % (1 << 8)) & 0xff;
        for (int j = 0; j < 2; j++)
        {
            str1.insert(0,1,str16[b % 16]);
            b = b / 16;
        }
        str += str1;
    }
    return str;
}

string getMD5(string source)
{
    atemp = A;
    btemp = B;
    ctemp = C;
    dtemp = D;
    unsigned int * strByte = add(source);

    for(unsigned int i = 0;i < strlength / 16; i++)
    {
        unsigned int num[16];
        for(unsigned int j = 0;j < 16;j++)
            num[j] = strByte[i * 16 + j];
        mainLoop(num);
    }
    return changeHex(atemp).append(changeHex(btemp)).append(changeHex(ctemp)).append(changeHex(dtemp));
}


class BaseTask
{
    public:
        virtual void doit() = 0;
};

class Task : public BaseTask
{
    private:
        int sockfd;
        int epollfd;
    public:

        Task(int fd,int epofd) : sockfd(fd),epollfd(epofd)
    {

    }
        static int print(void *data,int args_num,char **argv,char **argc)
        {
            for(int i = 0; i < args_num; i++)
            {
                cout<<argc[i]<<" = " <<(argv[i]?argv[i]:"NULL")<<"\t";
            }
            *(int*)data = args_num;
            cout<<endl;

            return 0;

        }
        void doit()
        {
            int count = 0;
            char buffer[MAX_BUFFER];
            char buffer2[MAX_BUFFER];

            memset(buffer,'\0', sizeof(buffer)); 
            memset(buffer2,'\0', sizeof(buffer2)); 
            
            //users[sockfd].timer->expire = time(NULL) + TIMESLOT;
            //client_time_heap.percolate_down(0);
            while(1)
            {
                int n = read(sockfd, buffer2 + count, 30);
                if(n == 0) 
                {
                    struct epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = sockfd;
                    //epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, &ev);
                    //shutdown(sockfd, SHUT_RDWR);
                    printf("someone has logout,he's sockfd:%d\n", sockfd);
                    new_increace[sockfd] = 2;
                    //users[sockfd].judge = 4;
                    break; 
                }
                else if (n < 0 && EAGAIN == errno) {
                    struct epoll_event ev;
                    ev.data.fd = sockfd;
                    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    epoll_ctl( epollfd, EPOLL_CTL_MOD, sockfd, &ev);
                    break;
                }
                else if (n > 0) {
                    count += n;
                    if(count > MAX_BUFFER)
                    {
                        perror("buffer has not enough!");
                        break;
                    }
                } 
                else {
                    perror("read failure.");
                    break;
                }

            }

            Base64Decode(buffer,buffer2,strlen(buffer2));

            char *temp = new char[200];
            memset(temp, '\0', 200);
            write(1,buffer, MAX_BUFFER - 1);   

            if(buffer[0] == '1')
            {
                int error_remind;
                char *Error_Message;  
                char name[9];name[8] = '\0';
                char password[9];password[8] = '\0';
                char phone_number[12];
                phone_number[11] = '\0';
                memcpy(name,buffer + 1,8);
                memcpy(password,buffer + 9,8);
                memcpy(phone_number,buffer + 17, 11);
                sprintf(temp,"insert into user_1(name,password,phone_number) values ('%s','%s','%s')",name,getMD5(password).c_str(),phone_number);
                error_remind = sqlite3_exec(db,temp,0,0,&Error_Message);
                memset(temp,'\0',200);
                char content[50];
                memset(content,'\0',100);
                sprintf(content,"%s:create count",name); 
                sprintf(temp,"insert into log(content) values ('%s')",content);   
                sqlite3_exec(db,temp,0,0,&Error_Message);

                if(error_remind != 0)
                {
                    cout<<"insert fail"<<endl;
                    write(sockfd,"\ncount had exit\n",18);
                }else {
                    cout<<"insert data ok."<<endl;
                    write(sockfd,"\ncreate ok!\n",14);
                }
            }else if (buffer[0] == '2')
            {
                int error_remind;
                char *Error_Message;
                char name[9];name[8] = '\0';
                char password[9];password[8] = '\0';
                char phone_number[12];phone_number[11] = '\0';

                memcpy(name, buffer + 1, 8);
                memcpy(password, buffer + 9, 8);
                memcpy(phone_number,buffer+17,11);

                sprintf(temp,"update user_1 set password = '%s' where phone_number = '%s' and name = '%s' ",getMD5(password).c_str(),phone_number,name);
                error_remind = sqlite3_exec(db,temp,0,0,&Error_Message);
                if(error_remind != 0){
                    cout<<"modify fail"<<endl;
                    write(sockfd, "\nmodify fail!\n",13);
                }else
                    cout<<"mofify ok."<<endl;
                write(sockfd, "\nmodify ok!\n",14);

                char content[50];
                memset(temp, '\0', 200);
                memset(content, '\0', 100);
                sprintf(content,"%s:modify password",name); 
                sprintf(temp,"insert into log(content) values ('%s')",content);   
                sqlite3_exec(db,temp,0,0,&Error_Message);

            }
            else if(buffer[0] == '3')
            {

                int error_remind;
                char *Error_Message;
                char name[9];name[8] = '\0';
                char password[9];password[8] = '\0';
                char phone_number[12];phone_number[11] = '\0';

                memcpy(name,buffer + 1, 8);
                memcpy(password,buffer + 9, 8);
                memcpy(phone_number,buffer + 17, 11);
                sprintf(temp,"select *from user_1 where password ='%s' and name ='%s' ",getMD5(password).c_str(),name);
                int first = 0;
                auto *p = this->print;
                error_remind = sqlite3_exec(db,temp,p,(void *)&first,&Error_Message);
                char content[50];
                memset(temp,'\0',200);
                memset(content,'\0',100);
                sprintf(content,"%s:login count",name); 
                sprintf(temp,"insert into log(content) values ('%s')",content);   
                sqlite3_exec(db,temp,0,0,&Error_Message);

                if(first == 0){
                    cout<<"login fail"<<endl;
                    write(sockfd,"login fail!\n",12);
                }else
                {    
                    cout<<"login ok.\n"<<endl; 

                    FILE * fp = fopen("data.txt","r");
                    char buffer[1000];
                    fread(buffer,1,1000,fp);
                    write(sockfd,"\nlogin ok!\n",12);
                    write(sockfd,"\nthis is the video list(had encoded):\n",42);


                    char p[1000];
                    char p4[50];
                   
                    Base64Encode(p,buffer,strlen(buffer));
                    write(sockfd,p,strlen(p)); 
                    write(sockfd,"\n\nthis is the URL(had encoded):\n\n",38); 
                    Base64Encode(p4,"rtsp://192.168.72.136:8554/<filename>",38);
                    write(sockfd,p4,strlen(p4)); 
                }

            }
            delete []temp;
        }

};

class EpollServer
{
    private:
        bool is_stop;   //是否停止epoll_wait的标志
        int threadnum;   //线程数目
        int sockfd;     //监听的fd
        int port;      //端口
        int epollfd;    //Epoll的fd
        threadpool<BaseTask> *pool;   //线程池的指针
        epoll_event events[MAX_EVENT];  //epoll的events数组
        struct sockaddr_in bindAddr;   //绑定的sockaddr
    public://构造函数
        EpollServer(){}
        EpollServer(int ports, int thread) : is_stop(false) , threadnum(thread) ,
        port(ports), pool(NULL)
        {
        }
        ~EpollServer()
        {
            delete pool;
        }

        void init();

        void epoll();

        static int setnonblocking(int fd)
        {
            int old_option = fcntl(fd, F_GETFL);
            int new_option = old_option | O_NONBLOCK;
            fcntl(fd, F_SETFL, new_option);
            return old_option;
        }

        static void addfd(int epollfd, int sockfd, bool oneshot)
        {
            epoll_event event;
            event.data.fd = sockfd;
            event.events = EPOLLIN | EPOLLET;
            if(oneshot)
            {
                event.events |= EPOLLONESHOT;
            }
            epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);
            EpollServer::setnonblocking(sockfd);
        }

};

int _epollfd;
void EpollServer::init()   //EpollServer的初始化
{
    bzero(&bindAddr, sizeof(bindAddr));
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_port = htons(port);
    bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //创建Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        printf("EpollServer socket init error\n");
        return;
    }
    int ret = bind(sockfd, (struct sockaddr *)&bindAddr, sizeof(bindAddr));
    if(ret < 0)
    {
        printf("EpollServer bind init error\n");
        return;
    }
    ret = listen(sockfd, 1024);
    if(ret < 0)
    {
        printf("EpollServer listen init error\n");
        return;
    }
    epollfd = epoll_create(1024);
    _epollfd = epollfd;
    if(epollfd < 0)
    {
        printf("EpollServer epoll_create init error\n");
        return;
    }
    pool = new threadpool<BaseTask>(threadnum);  //创建线程池
}


void cb_func2(client_data *user_data)
{
    epoll_ctl(user_data->epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    close(user_data->sockfd); 
    printf("fd:%d is idle,be closed !\n", user_data->sockfd);
}



void EpollServer::epoll()
{
    pool->start();   //线程池启动
    addfd(epollfd, sockfd, false);
    while(!is_stop)
    {//调用epoll_wait
        int ret = epoll_wait(epollfd, events, MAX_EVENT, -1);
        if(ret < 0)  //出错处理
        {
            printf("epoll_wait error\n");
            break;
        }
        for(int i = 0; i < ret; ++i)
        {
            int fd = events[i].data.fd;
            if(fd == sockfd)  //新的连接到来
            {
                struct sockaddr_in clientAddr;
                socklen_t len = sizeof(clientAddr);
                int confd = accept(sockfd, (struct sockaddr *)
                        &clientAddr, &len);

                char suggest1[100] = "\nwelcome to connect to the video server!!!\n\nplease choose what you want to do:\n\n";
                char suggest2[100] = "operation1:create new count\nlike this: 1 + your name(8) + password(8) + phone number(11)\n\n";
                char suggest3[100] = "operation2:modify the password\nlike this 2 + your name(8) + phone number(11) \n\n";
                char suggest4[100] = "operation3:login your count\nlike this: 3 + your name(8) + password(8)\n\n";
                write(confd, suggest1, strlen(suggest1));
                write(confd, suggest2, strlen(suggest2));
                write(confd, suggest3, strlen(suggest3));
                write(confd, suggest4, strlen(suggest4));

                EpollServer::addfd(epollfd, confd, true);
                
                /*
                users[confd].sockfd = confd;
                users[confd].epollfd = epollfd;
                heap_timer *timer = new heap_timer(TIMESLOT);
                timer->cb_func = cb_func;
                timer->user_data = &users[confd];                
                ret = client_time_heap.add_timer(timer);
                users[confd].timer = timer;
                */
                new_increace[confd] = 1;
                cout<<"new connect!\n";
            }
            else if(events[i].events & EPOLLIN)//某个fd上有数据可读
            {
                new_increace[fd] = 1;
                BaseTask *task = new Task(fd,epollfd);
                pool->append_task(task);
                cout<<"new message!\n";
            }
            else
            {
                printf("something else had happened\n");
            }

        }
    }
    close(sockfd);
    pool->stop();
}

#endif
