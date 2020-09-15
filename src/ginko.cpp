#include "ginko.h"

Ginko::Ginko(){
    HttpUserArray_ = new Http[MAX_FD];
  
    // root dir
    char serverPath[200];
    getcwd(serverPath, 200);
    char root[6] = "/root";
    Root_ = (char*)malloc(strlen(serverPath) + strlen(root) + 1);
    strcpy(Root_, serverPath);
    strcat(Root_, root);

    UsersTimerArray_ = new client_data[MAX_FD];
}

Ginko::~Ginko(){
    close(EpollFd_);
    close(ListenFd_);
    close(Pipefd_[0]);
    close(Pipefd_[1]);

    delete ThreadPool_;
    delete[] HttpUserArray_;
    delete[] UsersTimerArray_;
    free(Root_);
}

void Ginko::Init(
    int port, 
    string user, 
    string passwd, 
    string databaseName,
    int opt_linger, 
    int sql_num, 
    int thread_num)
{
    cout << "Ginko::init()" << endl;
    Port_ = port;
    SqlUser_ = user;
    SqlPasswd_ = passwd;
    SqlName_ = databaseName;
    OptLinger_ = opt_linger;
    SqlNum_ = sql_num;
    ThreadNum_ = thread_num;
}


void Ginko::SqlPoolInit(){
    cout << "Ginko::SqlPoolInit()" << endl;
    ConnPool_ = SqlPool::GetInstance();
    ConnPool_->init("localhost",3306, SqlUser_, SqlPasswd_, SqlName_,SqlNum_);
    HttpUserArray_->initmysql_result(ConnPool_);
}


void Ginko::ThreadPoolInit(){
    cout << "Ginko::ThreadPoolInit()" << endl;
    ThreadPool_ = new ThreadPool(ConnPool_, ThreadNum_);
}


void Ginko::EventListen(){
    cout << "Ginko::EventListen()" << endl;
    ListenFd_  = socket(PF_INET, SOCK_STREAM, 0);
    assert(ListenFd_ >= 0);
    // SO_LINGER 延时关闭，优雅关闭
    if(OptLinger_ == 0){
        struct linger temp = {0,1};
        setsockopt(ListenFd_, SOL_SOCKET, SO_LINGER, &temp, sizeof(temp));
    }else if(OptLinger_ == 1){
        struct linger temp = {1,1};
        setsockopt(ListenFd_, SOL_SOCKET, SO_LINGER, &temp, sizeof(temp));
    }

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(Port_);

    int flag = 1;
    // SO_REUSEADDR
    setsockopt(ListenFd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    ret = bind(ListenFd_, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);
    ret = listen(ListenFd_, 5);
    assert(ret >= 0);

    Utils::u_pipefd = Pipefd_;
    Utils::u_epollfd = EpollFd_;
    Utils_.init(TimerList_, TIMESLOT);

    epoll_event events[MAX_EVENT_NUMBER];
    EpollFd_ = epoll_create(5);
    assert(EpollFd_ != -1);
    Utils_.addfd(EpollFd_, ListenFd_, false);
    Http::m_epollfd = EpollFd_;

    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, Pipefd_);
    assert(ret != -1);
    Utils_.setnonblocking(Pipefd_[1]);
    Utils_.addfd(EpollFd_, Pipefd_[0], false);

    Utils_.addsig(SIGPIPE, SIG_IGN);
    Utils_.addsig(SIGALRM, Utils_.sig_handler, false);
    Utils_.addsig(SIGTERM, Utils_.sig_handler, false);

    alarm(TIMESLOT);
}


void Ginko::TimerInit(int connfd, struct sockaddr_in client_address){
    // init需要完善一下
    HttpUserArray_[connfd].init(connfd, client_address, Root_, 0, 0, 0, SqlUser_, SqlPasswd_, SqlName_);
    UsersTimerArray_[connfd].address = client_address;
    UsersTimerArray_[connfd].sockfd = connfd;
    // a new util_timer
    util_timer *timer = new util_timer;
    timer->user_data = &UsersTimerArray_[connfd];
    timer->cb_func = cb_func;
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    UsersTimerArray_[connfd].timer = timer;
    TimerList_.add_timer(timer);
}

// 有数据传输，移动3个slot
void Ginko::AdjustTimer(util_timer *timer){
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    TimerList_.adjust_timer(timer);
    printf("Ginko::AdjustTimer()\n");
}


void Ginko::DealTimer(util_timer* timer,int sockfd){
    timer->cb_func(&UsersTimerArray_[sockfd]);
    if(timer != nullptr)
        TimerList_.del_timer(timer);
    printf("Ginko::AdjustTimer(),close fd %d\n",UsersTimerArray_[sockfd].sockfd);
}

bool Ginko::DealClientData(){
    printf("Ginko::DealClientData()\n");
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    // while (1){
    //     int connfd = accept(ListenFd_, (struct sockaddr *)&client_address, &client_addrlength);
    //     printf("--->Ginko::DealClientData()，accept a new connection,connfd = %d\n",connfd);
    //     if (connfd < 0){
    //         printf("--->Ginko::DealClientData()，errno is:%d accept error\n", errno);
    //         break;
    //     }
    //     if (Http::UserCount >= MAX_FD){
    //         Utils_.show_error(connfd, "--->Internal server busy\n");
    //         printf("--->Internal server busy\n");
    //         break;
    //     }
    //     TimerInit(connfd, client_address);
    // }

    int connfd = accept(ListenFd_, (struct sockaddr *)&client_address, &client_addrlength);
    printf("    accept a new connection,connfd = %d\n",connfd);
    if (connfd < 0)
    {
        printf("errno is:%d accept error\n", errno);
        return false;
    }
    if (Http::m_user_count >= MAX_FD)
    {
        Utils_.show_error(connfd, "Internal server busy");
        printf("Internal server busy\n");
        return false;
    }
    TimerInit(connfd, client_address);
    return false;
}

bool Ginko::DealSignal(bool& timeout, bool& stop_server){
    printf("Ginko::DealSignal(), received signal SIGALRM\n");
    int ret = 0;
    int sig;
    char signals[1024];
    ret = recv(Pipefd_[0], signals, sizeof(signals), 0);
    if (ret == -1){
        return false;
    }else if (ret == 0){
        return false;
    }else{
        for (int i = 0; i < ret; ++i){
            switch (signals[i])
            {
                case SIGALRM: //14
                {
                    timeout = true;
                    printf("--->Ginko::DealSignal(), received signal SIGALRM\n");
                    break;
                }
                case SIGTERM: //15
                {
                    stop_server = true;
                    printf("--->Ginko::DealSignal(), received signal SIGTERM\n");
                    break;
                }
            }
        }
    }
    return true;
}

void Ginko::DealRead(int sockfd){
    printf("Ginko::DealRead()\n");
    util_timer *timer = UsersTimerArray_[sockfd].timer;
    //proactor
    if (HttpUserArray_[sockfd].read_once()){
        printf("--->Ginko::DealRead(), deal with read client(%s),sockfd = %d\n", 
                inet_ntoa(HttpUserArray_[sockfd].get_address()->sin_addr),sockfd);
        bool ret = ThreadPool_->append(HttpUserArray_ + sockfd);
        assert(ret == true);
        if(timer)  AdjustTimer(timer);
    }else{
        DealTimer(timer, sockfd);
    }
}


void Ginko::DealWrite(int sockfd){
    printf("Ginko::DealWrite()\n");
    util_timer *timer = UsersTimerArray_[sockfd].timer;
    //proactor
    if (HttpUserArray_[sockfd].write())
    {
        printf("--->Ginko::DealWrite(), deal with write client(%s),sockfd = %d\n", 
                inet_ntoa(HttpUserArray_[sockfd].get_address()->sin_addr),sockfd);
        if(timer) AdjustTimer(timer);
    }else {
        DealTimer(timer, sockfd);
    }
}

void Ginko::EventLoop(){
    cout << "Ginko::EventLoop()" << endl;
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server){
        int number = epoll_wait(EpollFd_, EventsArray_, MAX_EVENT_NUMBER, -1);
        printf("--->Ginko::EventLoop(), epoll_wait,number = %d\n",number);
        if (number < 0 && errno != EINTR){
            printf("--->epoll failure\n");
            break;
        }
        for(int i = 0; i < number; i++){
            int sockfd = EventsArray_[i].data.fd;
            if(sockfd == ListenFd_){
                bool flag = DealClientData();
                if (false == flag) continue;
            }else if (EventsArray_[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                util_timer *timer = UsersTimerArray_[sockfd].timer;
                DealTimer(timer, sockfd);
            }else if((sockfd == Pipefd_[0]) && (EventsArray_[i].events & EPOLLIN)){
                bool flag = DealSignal(timeout, stop_server);
                if (false == flag) continue;
            }else if(EventsArray_[i].events & EPOLLIN){
                DealRead(sockfd);
            }else if(EventsArray_[i].events & EPOLLOUT){ 
                DealWrite(sockfd);
            }
        }
        if(timeout){
            Utils_.timer_handler();
            printf("--->Ginko::EventLoop(), timer tick\n");
            timeout = false;
        }
    }
}



