#ifndef __TIME_HEAD__
#define __TIME_HEAD__
 
#include <stdio.h>
#include <netinet/in.h>
#include <time.h>
#include <atomic>
 
using namespace std;
extern atomic_int new_increace[1024];

#define BUFFER_SIZE 64
 
class heap_timer;
 
//客户端数据
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    heap_timer *timer;
    int epollfd;
    int judge;
};
 
//定时器
class heap_timer
{
public:
    heap_timer(int delay)
    {
        expire = time(NULL) + delay;
    }
 
public:
    time_t expire;                  //定时器生效的绝对时间
    void (*cb_func)(client_data*);  //定时器的回调函数
    client_data *user_data;         //客户端数据
};
 
//时间堆
class time_heap
{
public:
    //构造之一：初始化一个大小为cap的空堆
    time_heap(int cap);
 
    //构造之二：用已用数组来初始化堆
    time_heap(heap_timer **init_array, int size, int capacity);
    
    //销毁时间堆
    ~time_heap();
 
    //添加定时器timer
    int add_timer(heap_timer *timer);
    
    //删除定时器timer
    void del_timer(heap_timer *timer);
 
    //获得堆顶部的定时器
    heap_timer* top() const;
 
    //删除堆顶部的定时器
    void pop_timer();
 
    //心跳函数
    void tick();
 
    //堆是否为空
    bool empty()const { return cur_size == 0; }
   
public:
    //最小堆的下虑操作，
    //确保堆数组中认第hole个节点作为根的子树拥有最小堆性质
    void percolate_down(int hole);
    
    //将堆数组容量扩大1倍
    void resize();
private:
    heap_timer **array;     //堆数组
    int capacity;           //堆数组的空量
    int cur_size;           //堆数组当前包含元素个数
 
};

int do_error(int fd, int *error);       //处理错误
int setnonblocking(int fd);             //设置非阻塞
int addfd(int epollfd, int fd);         //添加描述符事件
void sig_handler(int sig);              //信号处理函数
void addsig(int sig);                   //添加信号处理函数
void timer_handler();                   //定时器任务
void cb_func(client_data *user_data);   //定时器回调函数
#endif

