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

/*
* related variables
*/
locker Lock;
std::map<std::string,std::string> Users;
int Http::UserCount_ = 0;
int Http::EpollFd_ = -1;


/* 
* global functions
*/
int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd, bool one_shot){
    epoll_event event;
    event.data.fd = fd;
   
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void modfd(int epollfd, int fd, int ev){
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

/*
* public member functions
*/

Http::Http(){
}
Http::~Http(){
}

void Http::Init(int sockfd, const sockaddr_in &addr, char *root, 
        std::string user, std::string passwd, std::string sqlname)
{
    SockFd_ = sockfd;
    Address_ = addr;
    addfd(EpollFd_, SockFd_, true);
    UserCount_ ++;
    DocRoot_ = root;
    strcpy(SqlUser_, user.c_str());
    strcpy(SqlPasswd_, passwd.c_str());
    strcpy(SqlName_, sqlname.c_str());

}

void Http::CloseConn(bool real_close){
    if(real_close && (SockFd_ != -1)){
        printf("close fd %d\n", SockFd_);
        removefd(EpollFd_, SockFd_);
        SockFd_ = -1;
        UserCount_ --;
    }
}


// 数据库相关,User在文件最前面定义
// std::map<std::string,std::string> Users;
void  Http::InitMysqlResult(SqlPool* connPool){
    MYSQL* mysql = nullptr;
    SqlRAII mysqlconn(&mysql,connPool);
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        printf("SELECT error:%s\n", mysql_error(mysql));
    }
    MYSQL_RES *result = mysql_store_result(mysql);
    int num_fields = mysql_num_fields(result);
    MYSQL_FIELD *fields = mysql_fetch_fields(result);
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        Users[temp1] = temp2;
    }
}

// private Init()
void Http::Init(){
    HttpMysql_     = nullptr;
    BytesToSend_   = 0;
    BytesHaveSend_ = 0;
    CheckState_    = CHECK_STATE_REQUESTLINE;
    Linger_ = false;
    Method_ = GET;
    Url_    = 0;
    Version_       = 0;
    ContentLength_ = 0;
    Host_         = 0;
    StartLine_    = 0;
    CheckedIndex_ = 0;
    ReadIndex_    = 0;
    WriteIndex_   = 0;
    State_        = 0;
    TimerFlag_    = 0;
    Improv_       = 0;
}

bool Http::Readonce(){
    return NO_REQUEST;
}
bool Http::Write(){
    return NO_REQUEST;
}

/*
* 主要的业务逻辑
* 每个线程都会调用Process这个函数
* 主要是解析这个HTTP请求
* 然后构造iovc[0]的元素(HTTP响应报文)
* 
*  Process()
*    └───ProcessRead()
*    |       └───ParseLine()
*    |       └───GetLine()
*    |       └───ParseRequestLine()
*    |       └───ParseHeader()
*    |       └───ParseContent()
*    |       └───DoRequest()**
*    |
*    └───ProcessWrite()
*    |       └───AddStatusLine()
*    |       └───AddHeaders()
*    |       └───AddContent()
*    |      
*    |
*    └───CloseConn()       
*/
void Http::Process(){

}

Http::HTTP_CODE Http::ProcessRead(){
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;
    while ((CheckState_ == CHECK_STATE_CONTENT && line_status == LINE_OK) || ((line_status = ParseLine()) == LINE_OK))
    {
        text = GetLine();
        StartLine_ = CheckedIndex_;
        printf("%s\n", text);
        switch (CheckState_)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret = ParseRequestLine(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        }
        case CHECK_STATE_HEADER:
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
        case CHECK_STATE_CONTENT:
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


// ParseLine()为从状态机，用于分析一行内容
// 返回值为行的读取状态:LINE_OK,LINE_BAD,LINE_OPEN
// LINE_OK表示读取到一个完整的行、LINE_OPEN表示行数据尚不完整
// LINE_BAD表示行出错了，可能是空格字符的问题，反正不符合请求行或者头部字段的格式
// 代码测试见test文件夹，parse_line_test.cpp
Http::LINE_STATUS Http::ParseLine(){
    char temp;
    for (; CheckedIndex_ < ReadIndex_; ++CheckedIndex_)
    {
        temp = ReadBuf_[CheckedIndex_];
       // cout << temp << endl;
        if (temp == '\r')
        {
            if ((CheckedIndex_ + 1) == ReadIndex_)
                return LINE_OPEN;
            else if (ReadBuf_[CheckedIndex_ + 1] == '\n')
            {
                ReadBuf_[CheckedIndex_++] = '\0';
                ReadBuf_[CheckedIndex_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD; 
        }
        else if (temp == '\n')
        {
            if (CheckedIndex_ > 1 && ReadBuf_[CheckedIndex_ - 1] == '\r')
            {
                ReadBuf_[CheckedIndex_ - 1] = '\0';
                ReadBuf_[CheckedIndex_++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

char* Http::GetLine(){
    return ReadBuf_ + StartLine_;
}

Http::HTTP_CODE Http::ParseRequestLine(char* text){
    // 检索第一个匹配" "或者"\t""
    Url_ = strpbrk(text, " \t");
    if (!Url_)
        return BAD_REQUEST;
   
    *Url_++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
        Method_ = GET;
    else if (strcasecmp(method, "POST") == 0)
        Method_ = POST;
    else
        return BAD_REQUEST;

    Url_ += strspn(Url_, " \t");
    Version_ = strpbrk(Url_, " \t");
    if (!Version_)
        return BAD_REQUEST;
    *Version_++ = '\0';
    Version_ += strspn(Version_, " \t");
    if (strcasecmp(Version_, "HTTP/1.1") != 0)
        return BAD_REQUEST;

    if (strncasecmp(Url_, "http://", 7) == 0){
        Url_ += 7;
        Url_ = strchr(Url_, '/');
    }
    if (strncasecmp(Url_, "https://", 8) == 0){
        Url_ += 8;
        Url_ = strchr(Url_, '/');
    }

    if (!Url_ || Url_[0] != '/')
        return BAD_REQUEST;
    //当url为/时，显示判断界面
    if (strlen(Url_) == 1)
        // Url 变成了/judge.html
        strcat(Url_, "judge.html");
    CheckState_ = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

Http::HTTP_CODE Http::ParseHeader(char* text){
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
Http::HTTP_CODE Http::ParseContent(char* text){
    if (ReadIndex_ >= (ContentLength_ + CheckedIndex_)){
        text[ContentLength_] = '\0';
        //POST请求中最后为输入的用户名和密码
        String_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

Http::HTTP_CODE Http::DoRequest(){
    strcpy(RealFile_, DocRoot_);
    int len = strlen(DocRoot_);
    // 找到最后一个的 / 的位置
    const char *p = strrchr(Url_, '/');

    if ((*(p + 1) == '2' || *(p + 1) == '3')){
        char *url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real, "/");
        strcat(url_real, Url_ + 2);
        strncpy(RealFile_ + len, url_real, FILENAME_LEN - len - 1);
        free(url_real);

        // 将用户名和密码提取出来
        // 比如POST请求实体主体是user=123&passwd=123
        char name[100], password[100];
        int i;
        for (i = 5; String_[i] != '&'; ++i)
            name[i - 5] = String_[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; String_[i] != '\0'; ++i, ++j)
            password[j] = String_[i];
        password[j] = '\0';

        if (*(p + 1) == '3'){
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            char *sql_insert = (char *)malloc(sizeof(char) * 200);
            strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
            strcat(sql_insert, "'");
            strcat(sql_insert, name);
            strcat(sql_insert, "', '");
            strcat(sql_insert, password);
            strcat(sql_insert, "')");

            if (Users.find(name) == Users.end()){
                Lock.lock();
                //注册的时候，把数据库和哈希表都锁起来
                //在哈希表和数据库中都插入新的用户名和密码
                int res = mysql_query(HttpMysql_, sql_insert);
                Users.insert(pair<string, string>(name, password));
                Lock.unlock();

                if (!res)
                    strcpy(Url_, "/log.html");
                else
                    strcpy(Url_, "/registerError.html");
            }
            else
                strcpy(Url_, "/registerError.html");
        }else if(*(p + 1) == '2'){
            //这时候只需要对哈希表进行判断，不需要再对数据库进行查询
            if (Users.find(name) != Users.end() && Users[name] == password)
                strcpy(Url_, "/welcome.html");//图片，视频
            else
                strcpy(Url_, "/logError.html");//登录错误界面    
        }
    }

    if (*(p + 1) == '0'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/register.html");
        strncpy(RealFile_ + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '1'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/log.html");
        strncpy(RealFile_ + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '5'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/picture.html");
        strncpy(RealFile_ + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '6'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/video.html");
        strncpy(RealFile_ + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '7'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/index.html");
        strncpy(RealFile_ + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }else
        strncpy(RealFile_ + len, Url_, FILENAME_LEN - len - 1);

    if (stat(RealFile_, &FileStat_) < 0)
        return NO_RESOURCE;// 后面要改成404
    if (!(FileStat_.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;// 后面要改成403
    if (S_ISDIR(FileStat_.st_mode))
        return BAD_REQUEST;      

    int fd = open(RealFile_, O_RDONLY);
    FileAddress_ = (char *)mmap(0, FileStat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    printf("m_file_address:%s\n",FileAddress_);//打印m_file_address
    close(fd);
    return FILE_REQUEST;
}

/* 
* response 
*/
bool Http::ProcessWrite(HTTP_CODE ret){
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        AddStatusLine(500, error_500_title);
        AddHeaders(strlen(error_500_form));
        if (!AddContent(error_500_form))
            return false;
        break;
    }
    case BAD_REQUEST:
    {
        AddStatusLine(404, error_404_title);
        AddHeaders(strlen(error_404_form));
        if (!AddContent(error_404_form))
            return false;
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        AddStatusLine(403, error_403_title);
        AddHeaders(strlen(error_403_form));
        if (!AddContent(error_403_form))
            return false;
        break;
    }
    case FILE_REQUEST:
    {
        AddStatusLine(200, ok_200_title);
        if (FileStat_.st_size != 0)
        {
            AddHeaders(FileStat_.st_size);
            Iovec_[0].iov_base = WriteBuf_;
            Iovec_[0].iov_len = WriteIndex_;
            Iovec_[1].iov_base = FileAddress_;
            Iovec_[1].iov_len = FileStat_.st_size;
            IovecCount = 2;
            BytesToSend_ = WriteIndex_ + FileStat_.st_size;
            return true;
        }
        else
        {
            const char *ok_string = "<html><body></body></html>";
            AddHeaders(strlen(ok_string));
            if (!AddContent(ok_string))
                return false;
        }
    }
    default:
        return false;
    }
    Iovec_[0].iov_base = WriteBuf_;
    Iovec_[0].iov_len = WriteIndex_;
    IovecCount = 1;
    BytesToSend_ = WriteIndex_;
    return true;
}

void Http::Unmap(){
    if (FileAddress_)
    {
        munmap(FileAddress_, FileStat_.st_size);
        FileAddress_ = 0;
    }

}
bool Http::AddResponse(const char* format,...){
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

    printf("request:%s\n", WriteBuf_);
    return true;
}

bool Http::AddStatusLine(int status, const char* title){
    return AddResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}
// AddHeaders()
//     └───AddContentLength()
//     └───AddLinger()
//     └───AddBlankLine()
bool Http::AddHeaders(int content_length){
    return AddContentLength(content_length)
           && AddLinger()
           && AddBlankLine();
}
bool Http::AddContentLength(int content_length){
    return AddResponse("Content-Length:%d\r\n", content_length);
}
bool Http::AddLinger(){
    return AddResponse("Connection:%s\r\n", 
            (Linger_ == true) ? "keep-alive" : "close");
}
bool Http::AddBlankLine(){
    return AddResponse("%s", "\r\n");
}


bool Http::AddContentType(){
    return AddResponse("Content-Type:%s\r\n", "text/html");
}

bool Http::AddContent(const char* content){
    return AddResponse("%s", content);
}