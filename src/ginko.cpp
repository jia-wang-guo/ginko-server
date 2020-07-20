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

void Ginko::init(
    int port, 
    string user, 
    string passwd, 
    string databaseName,
    int opt_linger, 
    int sql_num, 
    int thread_num)
{
    Port_ = port;
    SqlUser_ = user;
    SqlPasswd_ = passwd;
    SqlName_ = databaseName;
    OptLinger_ = opt_linger;
    SqlNum_ = sql_num;
    ThreadNum_ = thread_num;
}


void Ginko::SqlPoolInit(){
    ConnPool_ = SqlPool::GetInstance();
    ConnPool_->init("localhost",3306, SqlUser_, SqlPasswd_, SqlName_,SqlNum_);
    HttpUserArray_->InitMysqlResult(ConnPool_);
}


void Ginko::ThreadPoolInit(){
    ThreadPool_ = new ThreadPool(ConnPool_, ThreadNum_);
}


void Ginko::EventListen(){
    ListenFd_  = socket(PF_INET, SOCK_STREAM, 0);
    assert(ListenFd_ >= 0);

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
    Http::EpollFd_ = EpollFd_;

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
    HttpUserArray_[connfd].Init(connfd, client_address, Root_, SqlUser_, SqlPasswd_, SqlName_);
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
    printf("adjust timer once\n");
}


void Ginko::DealTimer(util_timer* timer,int sockfd){
    timer->cb_func(&UsersTimerArray_[sockfd]);
    if(timer != nullptr)
        TimerList_.del_timer(timer);
    printf("deal_timer,close fd %d\n",UsersTimerArray_[sockfd].sockfd);
}

bool Ginko::DealClientData(){
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    while (1){
        int connfd = accept(ListenFd_, (struct sockaddr *)&client_address, &client_addrlength);
        printf("    accept a new connection,connfd = %d\n",connfd);
        if (connfd < 0){
            printf("errno is:%d accept error", errno);
            break;
        }
        if (Http::UserCount_ >= MAX_FD){
            Utils_.show_error(connfd, "Internal server busy");
            printf("Internal server busy");
            break;
        }
        TimerInit(connfd, client_address);
    }
    return false;
}

bool Ginko::DealSignal(bool& timeout, bool& stop_server){
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
                    printf("received signal SIGALRM\n");
                    break;
                }
                case SIGTERM: //15
                {
                    stop_server = true;
                    printf("received signal SIGTERM\n");
                    break;
                }
            }
        }
    }
    return true;
}

void Ginko::DealRead(int sockfd){
    util_timer *timer = UsersTimerArray_[sockfd].timer;
    //proactor
    if (HttpUserArray_[sockfd].Readonce()){
        printf("deal with read client(%s),sockfd = %d\n", 
                inet_ntoa(HttpUserArray_[sockfd].GetAddress()->sin_addr),sockfd);
        ThreadPool_->append(HttpUserArray_ + sockfd);
        if (timer)  AdjustTimer(timer);
    }else{
        DealTimer(timer, sockfd);
    }
}


void Ginko::DealWrite(int sockfd){
    util_timer *timer = UsersTimerArray_[sockfd].timer;
    //proactor
    if (HttpUserArray_[sockfd].Write())
    {
        printf("deal with write client(%s),sockfd = %d\n", 
                inet_ntoa(HttpUserArray_[sockfd].GetAddress()->sin_addr),sockfd);
        if(timer) AdjustTimer(timer);
    }else {
        DealTimer(timer, sockfd);
    }
}

void Ginko::EventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server){
        int number = epoll_wait(EpollFd_, EventsArray_, MAX_EVENT_NUMBER, -1);
        printf("in eventLoop,call epoll_wait,number = %d\n",number);
        if (number < 0 && errno != EINTR){
            printf("epoll failure");
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
            printf("timer tick\n");
            timeout = false;
        }
    }
}



