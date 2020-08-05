#ifndef __GINKO_H__
#define __GINKO_H__

// linux
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

// std
#include <cassert>
#include <cstdlib>
#include <cstdio>

#include "../net/http.h"
#include "../os/thread.h"
#include "../os/timer.h"


const int MAX_FD = 65536;
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 5;

class Ginko
{
public:
    Ginko();
    ~Ginko();
    void Init(int port, string user, string passwd, string databaseName,
            int opt_linger, int sql_num, int thread_num);
    void SqlPoolInit();
    void ThreadPoolInit();
    void EventListen();
    void EventLoop();
    void TimerInit(int connfd,struct sockaddr_in client_addr);

    void AdjustTimer(util_timer* timer);
    void DealTimer(util_timer* timer,int sockfd);
    bool DealClientData();
    bool DealSignal(bool& timeout, bool& stop_server);
    void DealRead(int sockfd);
    void DealWrite(int sockfd);

public:
    int Port_;
    char* Root_;
    
    int Pipefd_[2];
    Http* HttpUserArray_;

    // db
    SqlPool* ConnPool_;
    string SqlUser_;
    string SqlPasswd_;
    string SqlName_;
    int SqlNum_;

    // threadpool
    ThreadPool * ThreadPool_;
    int ThreadNum_;

    // epoll
    epoll_event EventsArray_[MAX_EVENT_NUMBER];
    int ListenFd_;
    int EpollFd_;
    int OptLinger_;

    // timer
    sort_timer_lst TimerList_;
    client_data* UsersTimerArray_;
    Utils Utils_;
};
#endif