#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <iostream>
#include <cstring>
#include <string>
#include <regex>

#include "common.h"

using namespace std;

class Request{
public:
    void Init(const char* buf);
    LINE_STATUS ParseLine();
    HTTP_CODE ParseRequestLine(std::string &line);
    HTTP_CODE ParseHeader(char* text);
    HTTP_CODE ParseContent(char* text);
    HTTP_CODE DoRequest();
    char*     GetLine();

    HTTP_CODE ProcessRead_();



public:
    char* ReadBuf;



public:
    int ReadIndex_;  
    int CheckedIndex_;  
    int StartLine_;

    CHECK_STATE CheckState_; 
    METHOD Method_;   

    char* RealFile_; 
    char* Url_;
    char* Version_;
    char* Host_;
    int ContentLength_;
    bool Linger_;
    // POST请求的body
    char* String_;

};

#endif