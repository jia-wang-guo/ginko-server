#ifndef __REQUEST_H__
#define __REQUEST_H__

class HttpRequest
{
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
    // master state machine status
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    // slave state machine status
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
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

public:
    HttpRequest();
    ~HttpRequest();

public:
    bool ParseRequestLine_(char* text);
    HTTP_CODE ParseHeader_(char* text);
    HTTP_CODE ParseContent_(char* text);

    char* GetLine_();
    HTTP_CODE ParseLine();
};


#endif