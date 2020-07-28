#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>


class Response{
public:
    bool ProcessResponse(int response);
    void ResponseInit();
    void Unmap();
private:

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
    
};

#endif // !