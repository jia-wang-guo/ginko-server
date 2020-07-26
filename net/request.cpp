#include "request.h"


LINE_STATUS Request::ParseLine(){
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

HTTP_CODE Request::ParseRequestLine(std::string &line){
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
        CheckState_ = HEADER;
        return NO_REQUEST;
    }
    return BAD_REQUEST;
}

HTTP_CODE Request::ParseHeader(char* text){
    printf("Request::ParseHeader()\n");
    if (text[0] == '\0'){
        if (ContentLength_ != 0){
            CheckState_ = CONTENT;
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
HTTP_CODE Request::ParseContent(char* text){
    printf("Request::ParseContent()\n");
    if (ReadIndex_ >= (ContentLength_ + CheckedIndex_)){
        text[ContentLength_] = '\0';
        //POST请求中最后为输入的用户名和密码
        String_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

HTTP_CODE Request::ProcessRead_(){
    printf("---> Request::ProcessRead()\n");
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;
    while (
            (CheckState_ == CONTENT && 
            line_status == LINE_OK) || 
            ((line_status = ParseLine()) == LINE_OK)
        )
    {
        text = GetLine();
        StartLine_ = CheckedIndex_;
        std::string line(text,CheckedIndex_);
        switch (CheckState_)
        {
        case REQUESTLINE:
        {
            ret = ParseRequestLine(line);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        }
        case HEADER:
        {
            ret = ParseHeader(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            else if (ret == GET_REQUEST)
            {
                return DoRequest();
            }
            break;
        }
        case CONTENT:
        {
            ret = ParseContent(text);
            if (ret == GET_REQUEST)
                return DoRequest();
            line_status = LINE_OPEN;
            break;
        }
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;

}