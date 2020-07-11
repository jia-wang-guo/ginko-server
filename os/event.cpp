#include "event.h"


//谁会调用这个构造函数？？？
//在webserver.cpp里面
template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) : 
m_actor_model(actor_model),       //事件处理模型
m_thread_number(thread_number),   //线程数量
m_max_requests(max_requests), 
m_threads(NULL),m_connPool(connPool)
{
    if (thread_number <= 0 || max_requests <= 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads)
        throw std::exception();
    for (int i = 0; i < thread_number; ++i)
    {
        //将类的对象作为函数传递给静态函数(worker)
        //pthread_create第三个参数应该是一个静态函数，
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i]))//分离子线程，使得子线程在结束时，可以让系统回收线程资源
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}
template<typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;  //删除线程池数组
    m_stop = true;
}

//请求队列中添加任务
template<typename T>
bool threadpool<T>::append(T *request)
{
    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}


//静态函数work是每个线程的入口函数
//在work里面通过对象使用类的动态方法run
template<typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;//当前的线程池
    pool->run();//this->run()
    return pool;//return this
}

template<typename T>
void threadpool<T>::run()
{
    while (true)
    {
        
        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request)
            continue;
        printf("")

    }
}