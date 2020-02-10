#ifndef _LOCKER_H_
#define _LOCKER_H_

#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>



/*互斥 locker*/
class mutex_locker
{
    private:
        pthread_mutex_t m_mutex;

    public:
        mutex_locker()
        {
            if(pthread_mutex_init(&m_mutex, NULL) != 0)
                printf("mutex init error!");
        }
        ~mutex_locker()
        {
            pthread_mutex_destroy(&m_mutex);
        }

        bool mutex_lock()
        {
            return pthread_mutex_lock(&m_mutex) == 0;
        }
        bool mutex_unlock()
        {
            return pthread_mutex_unlock(&m_mutex) == 0;
        }
};

/*条件变量 locker*/
class cond_locker
{
    private:
        pthread_mutex_t m_mutex;
        pthread_cond_t m_cond;

    public:
        cond_locker()
        {
            if (pthread_mutex_init(&m_mutex, NULL) != 0)
                printf("mutex init error");

            if(pthread_cond_init(&m_cond, NULL) != 0) {
                pthread_mutex_destroy(&m_mutex);
                printf("cond init error");
            }
        }

        ~cond_locker()
        {
            pthread_mutex_destroy(&m_mutex);
            pthread_cond_destroy(&m_cond);
        }

        //等待条件变量
        bool wait()
        {
            int ans = 0;
            pthread_mutex_lock(&m_mutex);
            ans = pthread_cond_wait(&m_cond, &m_mutex);
            pthread_mutex_unlock(&m_mutex);
            return ans == 0;
        }

        //唤醒等待条件变量的线程
        bool signal()
        {
            return pthread_cond_signal(&m_cond) == 0;
        }

        //唤醒all等待条件变量的线程
        bool broadcast()
        {
            return pthread_cond_broadcast(&m_cond) == 0;
        }
};

#endif
