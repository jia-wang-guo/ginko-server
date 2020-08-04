#include "response.h"

// 状态码和状态原语
// 2XX
const char *ok_200_title = "OK";
const char *ok_204_title = "No Content";
const char *ok_206_title = "Partical Content";
// 3XX
const char *redirect_301_title = "Moved Permanently";
const char *redirect_302_title = "Found";
const char *redirect_303_title = "See Other";
const char *redirect_304_title = "Not Modified";
const char *redirect_307_title = "Temporary Redirect";
// 4XX
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_401_title = "Unauthorized";
const char *error_401_form = "Unauthorized.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
// 5XX
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";
const char *error_503_title = "Service Unavaliable";
const char *error_503_form = "There was an unusual problem serving the request file.\n";

#define WRITE_BUFFER_SIZE 2048


void Response::ResponseInit(){
    WriteIndex_ = 0;

}

int  Response::MapFile(char num, string url){
    
}


bool Response::ProcessResponse(int response){
    printf("Response::ProcessResponse()\n");
    if (response == 500){
        printf("------->INTERNAL_ERROR\n");
        AddStatusLine_(500, error_500_title);
        AddHeaders_(strlen(error_500_form));
        if (!AddContent_(error_500_form))
            return false;
    }else if(response == 400) {
        printf("------->BAD_REQUEST\n");
        AddStatusLine_(404, error_404_title);
        AddHeaders_(strlen(error_404_form));
        if (!AddContent_(error_404_form))
            return false;
    }else if(response == 403) {
        printf("------->FORBIDDEN_REQUEST\n");
        AddStatusLine_(403, error_403_title);
        AddHeaders_(strlen(error_403_form));
        if (!AddContent_(error_403_form))
            return false;
    }else if(response == 408) {
        printf("------->FILE_REQUEST\n");
        AddStatusLine_(200, ok_200_title);
        if (FileStat_.st_size != 0){
            AddHeaders_(FileStat_.st_size);
            Iovec_[0].iov_base = WriteBuf_;
            Iovec_[0].iov_len = WriteIndex_;
            Iovec_[1].iov_base = FileAddress_;
            Iovec_[1].iov_len = FileStat_.st_size;
            IovecCount_ = 2;
            BytesToSend_ = WriteIndex_ + FileStat_.st_size;
            return true;
        } else {
            printf("ok_string\n");
            const char *ok_string = "<html><body></body></html>";
            AddHeaders_(strlen(ok_string));
            if (!AddContent_(ok_string))
                return false;
        }
    } else {
        return false;
    }
    Iovec_[0].iov_base = WriteBuf_;
    Iovec_[0].iov_len = WriteIndex_;
    IovecCount_ = 1;
    BytesToSend_ = WriteIndex_;
    return true;
 }


void Response::Unmap(){
    printf("Response::Unmap()\n");
    if (FileAddress_)
    {
        munmap(FileAddress_, FileStat_.st_size);
        FileAddress_ = 0;
    }

}
bool Response::AddResponse_(const char* format,...){
    printf("--->Response::AddResponse_()\n");
    if (WriteIndex_ >= WRITE_BUFFER_SIZE)
        return false;
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(WriteBuf_ + WriteIndex_, WRITE_BUFFER_SIZE - 1 - WriteIndex_, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - WriteIndex_))
    {
        va_end(arg_list);
        return false;
    }
    WriteIndex_ += len;
    va_end(arg_list);
    LOG_INFO("request:%s", WriteBuf_);
    printf("--->--->Response::AddResponse_(),request:%s\n", WriteBuf_);
    return true;
}

bool Response::AddStatusLine_(int status, const char* title){
    printf("Response::AddStatusLine_()\n");
    return AddResponse_("%s %d %s\r\n", "HTTP/1.1", status, title);
}
// AddHeaders()
//     └───AddContentLength()
//     └───AddLinger()
//     └───AddBlankLine()
bool Response::AddHeaders_(int content_length){
    printf("Response::AddHeaders_()\n");
    return AddContentLength_(content_length)
           && AddLinger_()
           && AddBlankLine_();
}
bool Response::AddContentLength_(int content_length){
    printf("Response::AddContentLength_()\n");
    return AddResponse_("Content-Length:%d\r\n", content_length);
}

bool Response::AddLinger_(){
    printf("Response::AddLinger_()\n");
    return AddResponse_("Connection:%s\r\n", 
            (Linger_ == true) ? "keep-alive" : "close");
}
bool Response::AddBlankLine_(){
    printf("Response::AddBlankLine_()\n");
    return AddResponse_("%s", "\r\n");
}


bool Response::AddContentType_(){
    printf("Response::AddContentType_()\n");
    return AddResponse_("Content-Type:%s\r\n", "text/html");
}

bool Response::AddContent_(const char* content){
    printf("Response::AddContent_()\n");
    return AddResponse_("%s", content);
}

