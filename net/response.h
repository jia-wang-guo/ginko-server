#ifndef __RESPONSE_H__
#define __RESPONSE_H__

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include "common.h"

class Response{
public:
    void Unmap();
    bool AddResponse(const char* format,...);
    bool AddContent(const char* content);
    bool AddStatusLine(int status, const char* title);
    bool AddHeaders(int content_length);
    bool AddContentType();
    bool AddContentLength(int content_length);
    bool AddLinger();
    bool AddBlankLine();
    void Init();

    bool ProcessWrite_(int response);


private:


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