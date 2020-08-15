# 线程池
线程池需要搭配工作队列一起使用，用信号量进行同步，使用锁进行互斥访问。

线程池的几个参数：

- ThreadNumber ：线程的数量,线程的数量一般设置为CPU的核心数目，这里设置为8

- MaxHttpRequest ：工作队列的大小，项目里设置的是10000

- ThreadsArray ：存放线程池每个线程id地址的数组

以下给出了简化版的线程池

```C++
class ThreadPool{
private:
    int ThreadNumber; // 线程数量
    int MaxRequest;   // 工作队列的规模
    pthread_t* ThreadArray;  // 线程池容器
    list<Http*> RequestWorkQueue; // 工作队列
    locker QueueLocker;  // 互斥锁
    sem QueueSem;   // 信号量
public:
    // 构造函数新建ThreadNumber个线程
    // 并把线程的状态设置为分离状态 
    ThreadPool(int thread_num, int max_request):
    ThreadNumber(thread_num),
    MaxRequest(max_request),
    ThreadArray(nullptr),
    {
    	if(thread_num <= 0 || max_requests <=0)
    		throw exception();
    	ThreadArray = new pthread_t[ThreadNumber];
    	if(ThreadArray == nullptr)
    		throw exception();
    	for(int i = 0; i < thread_num; ++i){
            if(pthread_create(ThreadsArray_ + i, NULL, worker, this) != 0){
                delete[] ThreadsArray_;
                throw std::exception();
            }
        }
        if(pthread_detach(ThreadsArray_[i]) != 0){
            delete[] ThreadsArray_;
            throw std::exception();            
        }
    }
    // 析构函数
    ~ThreadPool(){
    	delete[] ThreadArray;
    }
    // 主线程生产函数
    bool append(Http* request){
        // 在访问工作队列之前，要对其加锁
    	QueueLocker.lock();
    	if(RequestWorkQueue.size() >= MaxRequest){
    		QueueLocker.unlock();
    		return false;
    	}
    	RequestWorkQueue.push_back(request);
        // 一定要记得解锁，不然会造成死锁
    	QueueLocker.unlock();
        // 信号量V操作，可能会唤醒一个线程
    	QueueSem.V();
    }
    // pthread_create的第三个参数
    static worker(void* arg){
    	ThreadPool* pool = (ThreadPool*) arg;
    	pool->run();
    	return pool;
    }

    void run(){
    	while(1){
    		QueueSem.P();
    		QueueLocker.lock();
    		if(RequestWorkQueue.empty()){
    			QueueLocker.unlock();
    			continue;
    		}
    		Http* request = RequestWorkQueue.front();
    		RequestWorkQueue.pop_front();
    		QueueLocker.unlock();
    		// Process为http业务逻辑
    		request->Process();
    	}
    }

};
```