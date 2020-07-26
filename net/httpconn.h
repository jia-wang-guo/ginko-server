#ifndef __HTTPCONN_H__
#define __HTTPCONN_H__

#include <netinet/in.h> // sockaddr_in
#include <mysql/mysql.h>

#include "../db/sql.h"
#include "../os/locker.h"


class HttpConn{
public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;


public:
    static int EpollFd;
    static int UserCount;
    MYSQL* HttpMysql;

    //State 读为0，写为1
    int State; 
    int TimerFlag;
    int Improv;

private:
    int SockFd_;
    struct sockaddr_in Address_;
    char ReadBuf_[READ_BUFFER_SIZE];

};



#endif 