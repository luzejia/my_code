#include "time_heap.h"
#include <atomic>
#include <iostream>
#include <unistd.h>
#define FD_LIMIT 65535

using namespace std;
extern void cb_func2(client_data *user_data);
extern int _epollfd;
extern time_heap client_time_heap;
extern atomic_int new_increace[1024];
extern client_data *users ;

time_heap::time_heap(int cap)
    :capacity(cap), cur_size(0)
{
    //创建堆数组
    array = new heap_timer*[capacity];
    if (!array) {
        fprintf(stderr, "init heap_timer failed.\n");
        return;
    }
    
    for (int i = 0; i < capacity; ++i)
        array[i] = NULL;
}
 
time_heap::time_heap(heap_timer **init_array, int size, int capacity)
    :capacity(capacity), cur_size(size)
{
    if (capacity < size) {
        fprintf(stderr, "init heap_timer 1 for init_array failed.\n");
        return;
    }
 
    //创建堆数组
    array = new heap_timer*[capacity];
    if (!array) {
        fprintf(stderr, "init heap_timer 2 failed.\n");
        return;
    }
 
    for (int i = 0; i < capacity; ++i)
        array[i] = NULL;
 
    if (size != 0) {
        //初始化堆数组
        for (int i = 0; i < size; i++)
            array[i] = init_array[i];
 
        //对数组中第 (cur_size-1)/2 ~ 0 个元素执行下虑操作
        for (int i = (cur_size-1)/2; i >=0; --i)
            percolate_down(i);
    }
}
 
time_heap::~time_heap()
{
    for (int i = 0; i < cur_size; ++i)
        delete array[i];
 
    delete[] array;
}
 
int time_heap::add_timer(heap_timer *timer)
{
   if (!timer)
       return -1;
 
   //如果堆数组不够大，将其扩大1倍
   if (cur_size >= capacity)
       resize();

   //新插入一个元素，当前堆大小加1, hole是新建空穴的位置
   int hole = cur_size++;
   int parent = 0;

   //对从空穴到根节点的路上的所有节点执行上虑操作
   for (; hole > 0; hole = parent) {
       parent = (hole -1) /2;

       if (array[parent]->expire <= timer->expire)
           break;

       array[hole] = array[parent];
   }

   array[hole] = timer;

   return 0;
}
 
void time_heap::del_timer(heap_timer *timer)
{
    if (!timer)
        return;
 
    //仅将定时器的回调函数置为空，即所谓的延迟销毁。
    //这将节省真正删除该定时器的开销，但这样做容易使堆数组膨胀
    timer->cb_func = NULL;
}
 
heap_timer* time_heap::top() const
{
    if (empty())
        return NULL;
 
    return array[0];
}
 
void time_heap::pop_timer()
{
    if (empty())
        return;
 
    if (array[0]) {
        delete array[0];
 
        //将原来的堆顶元素替换为堆数组中最后一个元素
        array[0] = array[--cur_size];
 
        //对新的堆顶元素执行下虑操作
        percolate_down(0);
    }
}
 
void time_heap::tick()
{
     heap_timer *tmp = array[0];
    time_t cur = time(NULL);

    for (int i = 0; i < 1024; i++)
       {
            
            if (new_increace[i] && new_increace[i] != 2)
            {
                
                if (users[i].judge != 0)
                users[i].judge = 4;
                else 
                users[i].judge = 0;
            }else if (new_increace[i] && new_increace[i] == 2)
            {
                users[i].judge = 4;
            }
       }

    //循环处理到期定时器
    while (!empty()) {
        cout<<"i am here\n";
        if (!tmp)
            break;
        //如果堆顶定时期没到期，则退出循环
        if (tmp->expire > cur)
            break;
        if (tmp->user_data->judge == 3 )
        {
            write(array[0]->user_data->sockfd,"you had long time no answer,please connect again !\n",52); 
            if (array[0]->cb_func)
                array[0]->cb_func(array[0]->user_data);
            
            pop_timer();
        } else if (tmp->user_data->judge == 4)
        {
            if (new_increace[tmp->user_data->sockfd] == 2)
            {
                if (array[0]->cb_func)
                    array[0]->cb_func(array[0]->user_data);
            }
            pop_timer();
        } else
        {
            write(array[0]->user_data->sockfd,"please answer if you are online\n",32);
            tmp->user_data->judge++;
            tmp->expire += 10;
            percolate_down(0);
        }
        //删除堆元素，同时生成新的堆顶定时器
        tmp = array[0];
        
    }

    for (int i = 0; i < 1024; i++)
       {
            if (new_increace[i] && new_increace[i] != 2)
            {
                users[i].sockfd = i;
                users[i].judge = 0;
                users[i].epollfd = _epollfd;
                heap_timer *timer = new heap_timer(10);
                timer->cb_func = cb_func2;
                timer->user_data = &users[i];
                client_time_heap.add_timer(timer);
                users[i].timer = timer;
                new_increace[i] = 0;
            }else if (new_increace[i] && new_increace[i] == 2)
            {
                new_increace[i] = 0;
                users[i].judge = 0;
            }
       }
}
 
void time_heap::percolate_down(int hole)
{
   heap_timer *tmp = array[hole];
   int child = 0;
 
   for (; ((hole*2)+1) <= (cur_size-1); hole = child) {
       child = hole * 2 + 1;
 
       if (child < (cur_size-1) &&
               array[child+1]->expire < array[child]->expire) {
           ++child;
       }
 
       if (array[child]->expire < tmp->expire)
           array[hole] = array[child];
       else
           break;
   }
 
   array[hole] = tmp;
}
 
void time_heap::resize()
{
    heap_timer **tmp = new heap_timer*[2 * capacity];
 
    for (int i = 0; i < (2*capacity); ++i)
        tmp[i] = NULL;
 
    if (!tmp) {
        fprintf(stderr, "resize() failed.\n");
        return;
    }
 
    capacity = 2 * capacity;
 
    for (int i = 0; i < cur_size; ++i)
        tmp[i] = array[i];
 
    delete[] array;
    array = tmp;
}



