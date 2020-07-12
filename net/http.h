#ifndef __HTTP_H__
#define __HTTP_H__ 

//#include "./request.h"
//#include "./response.h"

// linux
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <mysql/mysql.h>
// std
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <map>
// user
#include "../db/sql.h"
#include "../os/locker.h"



class Http
{  
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
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
public:
    Http();
    ~Http();

public:
    void Init(int sockfd, const sockaddr_in &addr, char *, int, int, int, 
                std::string user, std::string passwd, std::string sqlname);
    void CloseConn(bool real_close = true);

    void Process();
    bool Readonce();
    bool Write();

    sockaddr_in* GetAddress() { return &Address_;}

    void  InitMysqlResult(SqlPool* connPool);
    void  InitResultFile(SqlPool* connPool);

private:
    void      Init();
    HTTP_CODE ProcessRead();
    bool      ProcessWrite(HTTP_CODE ret);

    bool      ParseRequestLine(char* text);
    HTTP_CODE ParseHeader(char* text);
    HTTP_CODE ParseContent(char* text);
    HTTP_CODE DoRequest();
    char*     GetLine();
    HTTP_CODE ParseLine();

    void Unmap();
    bool AddResponse(const char* format,...);
    bool AddContent(const char* content);
    bool AddStatusLine(int status, const char* title);
    bool AddHeaders(int content_length);
    bool AddContentType();
    bool AddContentLength(int content_length);
    bool AddLinger();
    bool AddBlankLine();


public:
    static int EpollFd_;
    static int UserCount_;
    MYSQL* HttpMysql_;
    int    State_;

private:
    int Sockfd_;
    sockaddr_in Address_;

    char ReadBuf_[READ_BUFFER_SIZE];
    int ReadIndex_;
    int CheckedIndex_;
    int StartLine_;

    char WriteBuf_[WRITE_BUFFER_SIZE];
    int WriteIndex_;

    CHECK_STATE CheckState_;
    METHOD Method_;

    char RealFile[FILENAME_LEN];
    char* Url_;
    char* Version_;
    char* Host_;
    int ContentLength_;
    bool Linger_;
    // for writev
    char* FileAddress_;
    struct stat FileStat_;
    struct iovec Iovec_[2];
    int IovecCount;
    char* String_;
    int BytesToSend_;
    int BytesHaveSend_;
    char* DocRoot;

    std::map<std::string,std::string> UserMap_;
    char SqlUser_[100];
    char SqlPasswd_[100];
    char SqlName_[100];
};

#endif