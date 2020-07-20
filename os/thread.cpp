#include "thread.h"

ThreadPool::ThreadPool(SqlPool *connPool, int thread_number, int max_requests):
    ThreadNumber_(thread_number),
    MaxHttpRequests_(max_requests),
    ThreadsArray_(nullptr),
    ConnPool_(connPool)
{
    if(thread_number <= 0 || max_requests <=0 )
        throw std::exception();
    ThreadsArray_ = new pthread_t[ThreadNumber_];
    if(ThreadsArray_ == nullptr)
        throw std::exception();
    for(int i = 0; i < thread_number; ++i){
        if(pthread_create(ThreadsArray_ + i, NULL, worker, this) != 0){
            delete[] ThreadsArray_;
            throw std::exception();
        }
        if(pthread_detach(ThreadsArray_[i])){
            delete[] ThreadsArray_;
            throw std::exception();            
        }
    }
}

ThreadPool::~ThreadPool(){
    delete[] ThreadsArray_;
}

bool ThreadPool::append(Http *request){
    QueueLocker_.lock();
    if(RequestWorkQueue_.size() >= MaxHttpRequests_){
        QueueLocker_.unlock();
        return false;
    }
    RequestWorkQueue_.push_back(request);
    QueueLocker_.unlock();
    Queuestat_.post();
    return true;
}

// worker()
//    └───run()
// 静态函数work是每个线程的入口函数
// 在work里面通过对象使用类的动态方法run     
void* ThreadPool::worker(void *arg){
    // current thread
    ThreadPool* pool = (ThreadPool*) arg;
    pool->run();
    return pool;
}

void ThreadPool::run(){
    // 每个线程的死循环
    while(1){
        Queuestat_.wait();
        QueueLocker_.lock();
        if(RequestWorkQueue_.empty()){
            QueueLocker_.unlock();
            continue;
        }
        // 线程每次从http请求队列队头取请求
        Http* request = RequestWorkQueue_.front();
        RequestWorkQueue_.pop_front();
        QueueLocker_.unlock();
        if(request == nullptr)
            continue;
        SqlRAII mysqlcon(&request->HttpMysql_, ConnPool_);    
    }
}


