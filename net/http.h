#ifndef __HTTP_H__
#define __HTTP_H__
// linux
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <sys/uio.h> 
#include <sys/mman.h>
#include <sys/epoll.h>
#include <netinet/in.h> 
#include <mysql/mysql.h>
// cpp
#include <iostream>
#include <cstring>
#include <string>
#include <regex>
#include <cassert>
#include <cstdio>
#include <unordered_map>

#include "../db/sql.h"
#include "../os/locker.h"
#include "../log/log.h"

class Http{
/*
* http
*/
public:
    static int   EpollFd;
    static int   UserCount;
    MYSQL*       HttpMysql;
    int          m_close_log;
public:
    // 以下设置为公有的函数时提供给服务器的接口,配合私有的请求结
    // Init(),CloseConn(),Process(),Read(),Write(),GetAddress(),CreateSqlCache
    void         Init(int sockfd, const sockaddr_in &addr, char *root, 
                 std::string user, std::string passwd, std::string sqlname);
    void         CloseConn(bool real_close = true);
    void         Process();    // process主要给工作线程使用
    bool         Read();       // Read和Write主要给主线程使用
    bool         Write();
    sockaddr_in* GetAddress();
    void         CreateSqlCache(SqlPool* connPool);
private:
    void         Init_();     // Http内部调用
private:
    int          SockFd_;
    struct       sockaddr_in Address_;
    char*        DocRoot_;
    char         SqlUser_[100];
    char         SqlPasswd_[100];
    char         SqlName_[100];
/*
* request
*/
public:
    static const int REALFILE_LEN  = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 2048;
    enum        METHOD { GET = 0,POST,HEAD,PUT,DELETE,TRACE,OPTIONS,CONNECT,PATH };
    enum        CHECK_STATE{ REQUESTLINE = 0,HEADER,CONTENT,FINISH };
    enum        LINE_STATUS{ LINE_OK = 0,LINE_BAD,LINE_OPEN };
public:
    int         ProcessRequest();
private:
    LINE_STATUS ReadLine_();
    char*       GetLine_();
    int         ParseRequestLine_(std::string &line);
    int         ParseHeader_(char* text);
    int         ParseContent_(char* text);
    int         FinishParse_();
private:  
    char        ReadBuf_[READ_BUFFER_SIZE];
    int         ReadIndex_;
    int         CheckedIndex_;  
    int         StartLine_;
    CHECK_STATE CheckState_; 
    METHOD      Method_;   
    char*       Url_;
    char*       Version_;
    char*       Host_;
    int         ContentLength_;
    bool        Linger_;
    char*       BodyString_; // http请求实体的内容
    char       RealFile_[REALFILE_LEN];  // 文件的真实路径
/*
* response
*/
public:
    bool   ProcessResponse(int response);
private:
    void   Unmap_();
    bool   AddResponse_(const char* format,...);
    bool   AddContent_(const char* content);
    bool   AddStatusLine_(int status, const char* title);
    bool   AddHeaders_(int content_length);
    bool   AddContentType_();
    bool   AddContentLength_(int content_length);
    bool   AddLinger_();
    bool   AddBlankLine_();
public:
    char   WriteBuf_[WRITE_BUFFER_SIZE];
    int    WriteIndex_; 
    int    BytesToSend_;
    int    BytesHaveSend_;
    char*  FileAddress_;    //文件的内存地址
    struct stat FileStat_;  
    struct iovec Iovec_[2];
    int    IovecCount_;


};

#endif 