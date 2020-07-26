#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <iostream>
#include <cstring>
#include <string>
#include <regex>
#include <cstdio>
#include <sys/stat.h> 
#include <sys/uio.h> 
#include <unistd.h> 
#include <fcntl.h>  
#include <sys/mman.h>
#include <unordered_map>
#include <mysql/mysql.h>
#include "../os/locker.h"

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
    void        Init(const char* buf);
    int         ProcessRead();
private:
    LINE_STATUS ReadLine_();
    char*       GetLine_();
    int         ParseRequestLine_(std::string &line);
    int         ParseHeader_(char* text);
    int         ParseContent_(char* text);
    int         FinishParse_();


public:
    // 要从httpconn传进来 
    char* ReadBuf;
    unordered_map<string,string>* SqlUser;
    MYSQL* HttpRequestMysql;
    struct stat FileStat;
    char* FileAddress;
private:
    int ReadIndex_;  
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