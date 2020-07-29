#ifndef __HTTPCONN_H__
#define __HTTPCONN_H__

#include <netinet/in.h> // sockaddr_in
#include <mysql/mysql.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>


#include "../db/sql.h"
#include "../os/locker.h"

#include "request.h"
#include "response.h"

class HttpConn{
public:
    static int EpollFd;
    static int UserCount;
    MYSQL* HttpMysql;
    int m_close_log;

public:
    HttpConn(){
        m_close_log = 0;
        Request_ = new Request;
        Response_ = new Response;
    }
    ~HttpConn(){
        delete Request_;
        delete Response_;
    }

public:

    void Init(int sockfd, const sockaddr_in &addr, char *root, 
        std::string user, std::string passwd, std::string sqlname);
    void CloseConn(bool real_close = true);
    void Process();
    bool Read();
    bool Write();
    sockaddr_in* GetAddress();
    void  CreateSqlCache(SqlPool* connPool);


private:
    void Init_();

private:
    int SockFd_;
    struct sockaddr_in Address_;
    char* DocRoot_;
    char SqlUser_[100];
    char SqlPasswd_[100];
    char SqlName_[100];

    bool Linger_;
    Request* Request_;
    Response* Response_;
};



#endif 