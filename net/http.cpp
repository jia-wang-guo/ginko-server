#include "http.h"



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

locker Lock;
std::map<std::string,std::string> Users;

Http::Http(){
    std::cout << "http construct" << std::endl;
}
Http::~Http(){
    std::cout << "http destruct" << std::endl;
}


void Http::Init(int sockfd, const sockaddr_in &addr, char *, int, int, int, 
            std::string user, std::string passwd, std::string sqlname)
{

}
void Http::CloseConn(bool real_close){

}


void Http::Process(){

}
bool Http::Readonce(){
    return NO_REQUEST;
}
bool Http::Write(){
    return NO_REQUEST;
}

void  Http::InitMysqlResult(SqlPool* connPool){

}
void  Http::InitResultFile(SqlPool* connPool){

}


void Http::Init(){

}
Http::HTTP_CODE Http::ProcessRead(){
    return NO_REQUEST;
}
bool Http::ProcessWrite(HTTP_CODE ret){
    return false;
}

bool Http::ParseRequestLine(char* text){
    return false;
}
Http::HTTP_CODE Http::ParseHeader(char* text){
    return NO_REQUEST;
}
Http::HTTP_CODE Http::ParseContent(char* text){
    return NO_REQUEST;
}
Http::HTTP_CODE Http::DoRequest(){
    return NO_REQUEST;
}
char* Http::GetLine(){
    return nullptr;
}
Http::HTTP_CODE Http::ParseLine(){
    return NO_REQUEST;
}
void Http::Unmap(){

}
bool Http::AddResponse(const char* format,...){
    return false;
}
bool Http::AddContent(const char* content){
    return false;
}
bool Http::AddStatusLine(int status, const char* title){
    return false;
}
bool Http::AddHeaders(int content_length){
    return false;
}
bool Http::AddContentType(){
    return false;
}
bool Http::AddContentLength(int content_length){
    return false;
}
bool Http::AddLinger(){
    return false;
}
bool Http::AddBlankLine(){
    return false;
}
