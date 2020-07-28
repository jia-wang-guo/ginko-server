#include "httpconn.h"

/*
* related variables
*/
locker Lock;
std::map<std::string,std::string> Users;
int HttpConn::UserCount = 0;
int HttpConn::EpollFd = -1;

/* 
* global functions
*/
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd, bool one_shot){
    epoll_event event;
    event.data.fd = fd;
   
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void modfd(int epollfd, int fd, int ev){
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}


// member function

void HttpConn::Init(
        int sockfd, 
        const sockaddr_in &addr, 
        char  *root, 
        std::string user, 
        std::string passwd, 
        std::string sqlname)
{
    SockFd_ = sockfd;
    Address_ = addr;
    addfd(EpollFd, SockFd_, true);
    UserCount ++;
    DocRoot_ = root;
    strcpy(SqlUser_, user.c_str());
    strcpy(SqlPasswd_, passwd.c_str());
    strcpy(SqlName_, sqlname.c_str());
    Request_ = new Request;
    Response_ = new Response;
}
 // 待定义
 void HttpConn::Init_(){

 }


// 主线程调用Read()和Write()用来处理IO
// 工作线程调用Process()用来处理业务逻辑
bool HttpConn::Read(){
    printf("--->HttpConn::Read()\n");
    if (Request_->ReadIndex >= 2048){
        return false;
    }
    int bytes_read = 0;
    while (true){
        bytes_read = recv(SockFd_, Request_->ReadBuf + Request_->ReadIndex, 
                             2048 - Request_->ReadIndex, 0);
        if (bytes_read == -1){
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            return false;
        } else if (bytes_read == 0){
            return false;
        }
        Request_->ReadIndex += bytes_read;
    }
    cout << "bytes_read = " << bytes_read << " ReadBuf = " <<  Request_->ReadBuf << endl;
    return true;
}

bool HttpConn::Write(){
    printf("---> Http::Write()\n");
    int temp = 0;
    // send all
    if (Response_->BytesToSend_ == 0){
        modfd(EpollFd, SockFd_, EPOLLIN);
        Init_();//初始化缓冲区相关的变量
        return true;
    }
    while (1){
        temp = writev(SockFd_, Response_->Iovec_, Response_->IovecCount_);
        if (temp < 0){
            if (errno == EAGAIN){
                modfd(EpollFd, SockFd_, EPOLLOUT);
                return true;
            }
            Response_->Unmap();
            return false;
        }
        Response_->BytesHaveSend_ += temp;
        Response_->BytesToSend_ -= temp;
        if (Response_->BytesHaveSend_ >= Response_->Iovec_[0].iov_len){
            Response_->Iovec_[0].iov_len = 0;
            Response_->Iovec_[1].iov_base = Response_->FileAddress_ + (Response_->BytesHaveSend_ - Response_->WriteIndex_);
            Response_->Iovec_[1].iov_len = Response_->BytesToSend_;
        } else {
            Response_->Iovec_[0].iov_base = Response_->WriteBuf_ + Response_->BytesHaveSend_;
            Response_->Iovec_[0].iov_len = Response_->Iovec_[0].iov_len - Response_->BytesHaveSend_;
        }
        //？？？为什么和这个函数最前面重复了呢
        if (Response_->BytesToSend_ <= 0){
            Response_->Unmap();
            modfd(EpollFd, SockFd_, EPOLLIN);
            if (Linger_){
                Init_();//初始化新的连接，为什么？？？？？
                return true;
            } else {
                return false;
            }
        }
    }
}

void HttpConn::Process(){
    printf("---> Http::Process()\n");
    int read_ret = Request_->ProcessRequest();
    if (read_ret == 100){
        modfd(EpollFd, SockFd_, EPOLLIN);
        return;
    }
    bool write_ret = Response_->ProcessResponse(read_ret);
    if (!write_ret){
        CloseConn();
    }
    modfd(EpollFd, SockFd_, EPOLLOUT);    
}

void HttpConn::CloseConn(bool real_close){
    if(real_close && (SockFd_ != -1)){
        printf("close fd %d\n", SockFd_);
        removefd(EpollFd, SockFd_);
        SockFd_ = -1;
        UserCount --;
    }
}


sockaddr_in* HttpConn::GetAddress() { 
    return &Address_; 
}

void  HttpConn::CreateSqlCache(SqlPool* connPool){
    MYSQL* mysql = nullptr;
    SqlRAII mysqlconn(&mysql,connPool);
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        printf("SELECT error:%s\n", mysql_error(mysql));
    }
    MYSQL_RES *result = mysql_store_result(mysql);
    int num_fields = mysql_num_fields(result);
    MYSQL_FIELD *fields = mysql_fetch_fields(result);
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        Users[temp1] = temp2;
    }
}

