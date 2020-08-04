#ifndef __HTTP_H__
#define __HTTP_H__

#include <netinet/in.h> 
#include <mysql/mysql.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>


#include "../db/sql.h"
#include "../os/locker.h"

#include "request.h"
#include "response.h"

class Http{
public:
    static int EpollFd;
    static int UserCount;
    MYSQL* HttpMysql;
    int m_close_log;
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
    // 每个连接特有的信息
    int SockFd_;
    struct sockaddr_in Address_;
    char* DocRoot_;
    char SqlUser_[100];
    char SqlPasswd_[100];
    char SqlName_[100];
    bool Linger_;
};



#endif 


#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <iostream>
#include <cstring>
#include <string>
#include <regex>
#include <cassert>
#include <cstdio>
#include <sys/stat.h> 
#include <sys/uio.h> 
#include <unistd.h> 
#include <fcntl.h>  
#include <sys/mman.h>
#include <unordered_map>
#include <mysql/mysql.h>
#include "../os/locker.h"
#include "../log/log.h"

using namespace std;

class Request{

public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD 
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        REQUESTLINE = 0,
        HEADER,
        CONTENT,
        FINISH
    };

    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    void        RequestInit();
    int         ProcessRequest();
private:
    LINE_STATUS ReadLine_();
    char*       GetLine_();
    int         ParseRequestLine_(std::string &line);
    int         ParseHeader_(char* text);
    int         ParseContent_(char* text);
    int         FinishParse_();


public:

    char ReadBuf[2048];
    int ReadIndex;
    int m_close_log;

private:  
    int CheckedIndex_;  
    int StartLine_;

    CHECK_STATE CheckState_; 
    METHOD      Method_;   

    char* DocRoot_;
    char* RealFile_; 
    char* Url_;
    char* Version_;
    char* Host_;
    int   ContentLength_;
    bool  Linger_;
    char* BodyString_;

    locker Lock_;
};

#endif

#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include "../log/log.h"

class Response{
public:
    bool ProcessResponse(int response);
    void ResponseInit();
    void Unmap();
private:
    int  MapFile(char num, string url);
    bool AddResponse_(const char* format,...);
    bool AddContent_(const char* content);
    bool AddStatusLine_(int status, const char* title);
    bool AddHeaders_(int content_length);
    bool AddContentType_();
    bool AddContentLength_(int content_length);
    bool AddLinger_();
    bool AddBlankLine_();



public:
    char* FileAddress_;
    struct stat FileStat_;
    struct iovec Iovec_[2];
    int IovecCount_;
    char* String_;
    int BytesToSend_;
    int BytesHaveSend_;
    char* DocRoot_;

    char* WriteBuf_;
    int WriteIndex_; 

    bool Linger_;

    int m_close_log;

    
    // 要从httpconn传进来 
    unordered_map<string,string>* Users;
    MYSQL* HttpRequestMysql;
    struct stat FileStat;
    char* FileAddress;


};

#endif // !