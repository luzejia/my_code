#ifndef _PTHREAD_POOL_
#define _PTHREAD_POOL_

#include "locker.h"
#include <queue>
#include <stdio.h>
#include <exception>
#include <errno.h>
#include <pthread.h>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include "time_heap.h"
#include <signal.h>

#define TIMESLOT 10 
#define FD_LIMIT 65535
#define MAX_EVENT_NUMBER 1024
#define TIMESLOT 10 
using namespace std;
extern time_heap client_time_heap;
extern client_data *users;
extern atomic_int new_increace[1024];

template<class T>
class threadpool
{
    private:
        int thread_number;
        pthread_t *all_threads;
        std::deque<T *> task_queue;
        mutex_locker queue_mutex_locker;       
        cond_locker queue_cond_locker;
        bool is_stop;
        static char is_work[50];
        char stop_to_work[50];
        int current_thread_num;

    public:
        threadpool(int thread_num = 10);
        ~threadpool();
        bool append_task(T *task);
        void start();
        void stop();
        struct thread_worker_parameter
        {
            threadpool *pool;
            int index;
        };

    private:
        static void *admin(void *arg);
        static void *worker(void *arg);
        static void *keepalive(void *arg);
        void run(int index);
        void control();
        void control2();
        T *getTask();
};

template<class T>char threadpool<T>::is_work[50];

template <class T>
threadpool<T>::threadpool(int thread_num):
    thread_number(thread_num),is_stop(false), all_threads(NULL),current_thread_num(thread_num)
{      
    if(thread_num <= 0)
        printf("threadpool can't init because thread_number = 0");

    all_threads = new pthread_t[thread_number];
    if(all_threads == NULL)
        printf("can't init threadpool because thread array can't new");
}

template <class T>
threadpool<T>::~threadpool()
{
    delete []all_threads;
    stop();
}

template <class T>
void threadpool<T>::stop()
{      
    queue_cond_locker.broadcast();
}

template <class T>
void threadpool<T>::start()
{
    for (int i = 0; i < thread_number; ++i)
    {
        thread_worker_parameter *temp = new thread_worker_parameter;
        temp->index = i;
        temp->pool = this;
            if(pthread_create(all_threads + i, NULL, worker, temp) != 0)
            {
                delete []all_threads;
                throw std::exception();
            }

            if(pthread_detach(all_threads[i]))
            {
                delete []all_threads;
                throw std::exception();
            }

        
    }

    thread_worker_parameter *temp = new thread_worker_parameter;
    temp->index = thread_number;
    temp->pool = this;
    if(pthread_create(all_threads + thread_number, NULL, admin, temp) != 0)
    {
        delete []all_threads;
        throw std::exception();
    }

    if(pthread_detach(all_threads[thread_number]))
    {
        delete []all_threads;
        throw std::exception();
    }

    thread_worker_parameter *temp2 = new thread_worker_parameter;
    temp->index = thread_number + 1;
    temp->pool = this;
    if(pthread_create(all_threads + thread_number + 1, NULL, keepalive, temp2) != 0)
    {
        delete []all_threads;
        throw std::exception();
    }

    if(pthread_detach(all_threads[thread_number + 1]))
    {
        delete []all_threads;
        throw std::exception();
    }
}

template <class T>
bool threadpool<T>::append_task(T *task)  
{   
    queue_mutex_locker.mutex_lock();

    bool is_signal = task_queue.empty();
   
    task_queue.push_back(task);
    queue_mutex_locker.mutex_unlock();
    
    if(is_signal)
    {
        queue_cond_locker.signal();
    }
    return true;
}

template <class T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = ((thread_worker_parameter*)arg)->pool;
    pool->run(((thread_worker_parameter*)arg)->index);
    return pool;
}

template <class T>
void *threadpool<T>::admin(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->control();
    return pool;
}

template <class T>
void *threadpool<T>::keepalive(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->control2();
    return pool;
}

template <class T>
T* threadpool<T>::getTask()
{

    T *task = NULL;
    queue_mutex_locker.mutex_lock();
    if(!task_queue.empty())
    {
        task = task_queue.front();
        task_queue.pop_front();
    }
    queue_mutex_locker.mutex_unlock();
    return task;
}

    template <class T>
void threadpool<T>::run(int index)
{   
    char a[20];
    sprintf(a,"I am NO.%d thread !\n",index);
    write(1,a,20);           
    while(!stop_to_work[index]){
        T *task = getTask();
        if(task == NULL)  
        {   
            is_work[index] = 0;
            queue_cond_locker.wait();
            is_work[index] = 1;
        }
        else            
        {
            is_work[index] = 1;
            task->doit();
          
        }
    }
}
    template <class T>
void threadpool<T>::control()
{
    int busy_thread = 0;
    int max_thread = 50;
    static int count_1 = 0;
    static int count_2 = 0;
    char p[11];
    char a[50];
    sprintf(a,"I am NO.%d thread for control the num of thread!\n",10);
    write(1,a,50);  
    while(!is_stop){
        busy_thread = 0;
        for(int i = 1;i < 50; ++i)
        {
            if(is_work[i] == 1) 
            {
                busy_thread++;
            }  
        }
        write(1,p,11);
        if ((((float)(busy_thread) / current_thread_num) >= 0.80) && (current_thread_num == 10.0))
        {
            char p[11];
            sprintf(p,"busy_num:%d",busy_thread);
            write(1,p,11);
            write(1,"pool is too busy,start to create new thread!",45);
            for(int i = 10; i < 15; ++i)
            {
               
                thread_worker_parameter *temp = new thread_worker_parameter;
                temp->index = i;
                temp->pool = this;
                if(pthread_create(all_threads + i, NULL, worker, temp) != 0)
                {         
                    delete []all_threads;
                    throw std::exception();
                }
                if(pthread_detach(all_threads[i]))
                {
                    delete []all_threads;
                    throw std::exception();
                }             
            }
            current_thread_num = 15;     

        } else if ((((float)(busy_thread) / current_thread_num) > 0.85) && (current_thread_num == 15.0)) 
        {
            write(1,"pool is too busy,start to create new thread!",45);
            for(int i = 15; i < 20; ++i)
            {
                
                thread_worker_parameter *temp = new thread_worker_parameter;
                temp->index = i;
                temp->pool = this;
                if(pthread_create(all_threads + i, NULL, worker, temp) != 0)
                {
                  
                    delete []all_threads;
                    throw std::exception();
                }
                if(pthread_detach(all_threads[i]))
                {
               
                    delete []all_threads;
                    throw std::exception();
                }
            }
            current_thread_num = 20;

        }
        else if(((busy_thread / 20.0) <= 0.5) && (current_thread_num == 20))
        {     
            count_1++;
            if(count_1 == 3)
            {
                for(int i = 15;i < 20;i++)
                {
                    stop_to_work[i] = 1;
                }
                queue_cond_locker.broadcast();
                write(1,"kill not busy thread!",22);
                current_thread_num = 15;
                count_1 = 0;
            }
        } else if(((busy_thread / 15.0) <= 0.35) && (current_thread_num == 15))
        {
            count_2++;
            if(count_2 == 3)
            {
                for(int i = 10;i < 15;i++)
                {
                    stop_to_work[i] = 1;
                }
                queue_cond_locker.broadcast();
                write(1,"kill not busy thread!",22);
                current_thread_num = 10;
                count_2 = 0;
            }
        }

        usleep(100000);
    }
}



 template <class T>
void threadpool<T>::control2()
{
    
    sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    
    char a[40];
    sprintf(a,"I am NO.%d thread for keepalive!\n",11);
    write(1,a,40); 
    
    while(1)
    {
            client_time_heap.tick();
	
        if (!client_time_heap.empty()) 
        {
            sleep(client_time_heap.top()->expire - time(NULL));
        }
        else
        {
            sleep(10);
            
        }
    } 
    
   
}



#endif


