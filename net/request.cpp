#include "request.h"

Request::LINE_STATUS Request::ParseLine(){
    printf("---> Request::ParseLine()\n");
    char temp;
    for (; CheckedIndex_ < ReadIndex_; ++CheckedIndex_)
    {
        temp = ReadBuf[CheckedIndex_];
       // cout << temp << endl;
        if (temp == '\r')
        {
            if ((CheckedIndex_ + 1) == ReadIndex_)
                return LINE_OPEN;
            else if (ReadBuf[CheckedIndex_ + 1] == '\n')
            {
                ReadBuf[CheckedIndex_++] = '\0';
                ReadBuf[CheckedIndex_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD; 
        }
        else if (temp == '\n')
        {
            if (CheckedIndex_ > 1 && ReadBuf[CheckedIndex_ - 1] == '\r')
            {
                ReadBuf[CheckedIndex_ - 1] = '\0';
                ReadBuf[CheckedIndex_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

char* Request::GetLine(){
    printf("---> Request::GetLine()\n");
    return ReadBuf + StartLine_;
}

Request::Request_CODE Request::ParseRequestLine(std::string &line){
    printf("---> Request::ParseRequestLine()\n");
    regex patten("^([^ ]*) ([^ ]*) Request/([^ ]*)$");
    smatch subMatch;
    string method, path, version;
    if(regex_match(line, subMatch, patten)) {   
        method = subMatch[1];
        path = subMatch[2];
        version = subMatch[3];
        cout << method <<"\t"<< path << "\t" << version <<endl;
        if(method == "GET")
            Method_ = GET;
        else if(method == "POST")
            Method_ = POST;
        else
            return BAD_REQUEST;
        // 这个地方后面需要优化一下
        Url_ = (char*)malloc(1024);
        memset(Url_,'\0',1024);
        strcpy(Url_, path.c_str());

        if (path == "/"){    
            strcpy(Url_, "/index.html");
        }
        CheckState_ = CHECK_STATE_HEADER;
        return NO_REQUEST;
    }
    return BAD_REQUEST;
}

Request::Request_CODE Request::ParseHeader(char* text){
    printf("Request::ParseHeader()\n");
    if (text[0] == '\0'){
        if (ContentLength_ != 0){
            CheckState_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0){
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0){
            Linger_ = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0){
        text += 15;
        text += strspn(text, " \t");
        ContentLength_ = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0){
        text += 5;
        text += strspn(text, " \t");
        Host_ = text;
    }
    else{
        printf("oop!unknow header: %s\n", text);
    }
    return NO_REQUEST;
}
// 只有POST请求会用到
Request::Request_CODE Request::ParseContent(char* text){
    printf("Request::ParseContent()\n");
    if (ReadIndex_ >= (ContentLength_ + CheckedIndex_)){
        text[ContentLength_] = '\0';
        //POST请求中最后为输入的用户名和密码
        String_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}
