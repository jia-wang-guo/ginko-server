#ifndef __RESPONESE_H__
#define __RESPONESE_H__

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

public:
    bool AddResponse_(const char* format,...);
    bool AddContent_(const char* content);
    bool AddStatusLine_(int status, const char* title);
    bool AddHeaders_(int content_length);
    bool AddContentType_();
    bool AddContentLength_(int content_length);
    bool AddLinger_();
    bool AddBlankLine_();
};

#endif