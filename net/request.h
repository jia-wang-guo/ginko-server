#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <cstring>
#include <string>
#include <regex>

using namespace std;

class Request{
public:
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
    void Init(const char* buf);
    LINE_STATUS ParseLine();
    HTTP_CODE ParseRequestLine(std::string &line);
    HTTP_CODE ParseHeader(char* text);
    HTTP_CODE ParseContent(char* text);
    HTTP_CODE DoRequest();
    char*     GetLine();

public:
    char* ReadBuf;

};

#endif