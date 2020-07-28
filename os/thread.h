#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"
#include "../db/sql.h"
#include "../net/httpconn.h"

class ThreadPool
{
public:
    ThreadPool(SqlPool *connPool, int thread_number = 8, 
                int max_request = 10000);
    ~ThreadPool();
    bool append(HttpConn *request);

private:
    static void *worker(void *arg);
    void run();

private:
    int ThreadNumber_;        
    int MaxHttpRequests_;         
    pthread_t *ThreadsArray_;       
    std::list<HttpConn *> RequestWorkQueue_; 
    locker QueueLocker_;       
    sem Queuestat_;            
    SqlPool *ConnPool_;      
};


#endif
