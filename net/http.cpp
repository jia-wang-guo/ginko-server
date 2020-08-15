#include "http.h"
/*
* related variables
*/
locker Lock;
std::map<std::string,std::string> Users;
int Http::UserCount = 0;
int Http::EpollFd = -1;
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


// member function

void Http::Init(
        int sockfd, 
        const sockaddr_in &addr, 
        char  *root, 
        std::string user, 
        std::string passwd, 
        std::string sqlname)
{
    printf("Http::Init()\n");
    DocRoot_ = root;
    SockFd_ = sockfd;
    Address_ = addr;
    addfd(EpollFd, SockFd_, true);
    UserCount ++;
    strcpy(SqlUser_, user.c_str());
    strcpy(SqlPasswd_, passwd.c_str());
    strcpy(SqlName_, sqlname.c_str());
    Init_();

}
 // 待定义
 void Http::Init_(){
    printf("Http::Init_()\n");
    HttpMysql = nullptr;
    
    // request
    ReadIndex_ = 0;
    CheckedIndex_ = 0;
    StartLine_ = 0;
    CheckState_ = REQUESTLINE;
    Method_ = GET;
    Url_ = nullptr;
    Version_ = nullptr;
    Host_ = nullptr;
    ContentLength_ = 0;
    Linger_ = false;

    memset(ReadBuf_, '\0', READ_BUFFER_SIZE);
    memset(WriteBuf_, '\0', WRITE_BUFFER_SIZE);
    memset(RealFile_, '\0', REALFILE_LEN);
}


// 主线程调用Read()和Write()用来处理IO
// 工作线程调用Process()用来处理业务逻辑
bool Http::Read(){
    printf("Http::Read()\n");
    if (ReadIndex_ >= 2048){
        return false;
    }
    int bytes_read = 0;
    while (true){
        memset(ReadBuf_ + ReadIndex_,'\0',2048 - ReadIndex_); 
        bytes_read = recv(SockFd_, ReadBuf_ + ReadIndex_, 2048 - ReadIndex_, 0);
        if (bytes_read == -1)  {
            if (errno == EAGAIN || errno == EWOULDBLOCK) 
                break;
            return false;
        } else if (bytes_read == 0){
            return false;
        }
        ReadIndex_ += bytes_read;
    }
    return true;
}

bool Http::Write(){
    printf("Http::Write()\n");
    int temp = 0;
    // 数据写完毕，清空写缓冲区
    if (BytesToSend_ == 0){
        modfd(EpollFd, SockFd_, EPOLLIN);
        Init_();//初始化缓冲区相关的变量
        return true;
    }
    while (1){
        temp = writev(SockFd_, Iovec_, IovecCount_);
        if (temp < 0){
            if (errno == EAGAIN){
                modfd(EpollFd, SockFd_, EPOLLOUT);
                return true;
            }
            Unmap_();
            return false;
        }
        BytesHaveSend_ += temp;
        BytesToSend_ -= temp;
        if (BytesHaveSend_ >= Iovec_[0].iov_len){
            Iovec_[0].iov_len = 0;
            Iovec_[1].iov_base = FileAddress_ + (BytesHaveSend_ - WriteIndex_);
            Iovec_[1].iov_len = BytesToSend_;
        } else {
            Iovec_[0].iov_base = WriteBuf_ + BytesHaveSend_;
            Iovec_[0].iov_len = Iovec_[0].iov_len - BytesHaveSend_;
        }
        // 为什么和这个函数最前面重复了呢
        if (BytesToSend_ <= 0){
            Unmap_();
            modfd(EpollFd, SockFd_, EPOLLIN);
            if (Linger_){
                Init_();//重新初始化新的连接
                return true;
            } else {
                return false;
            }
        }
    }
}

void Http::Process(){
    printf("Http::Process()\n");
    int read_ret = ProcessRequest();
    if (read_ret == 100){
        modfd(EpollFd, SockFd_, EPOLLIN);
        return;
    }
    bool write_ret = ProcessResponse(read_ret);
    if (!write_ret){
        CloseConn();
    }
    modfd(EpollFd, SockFd_, EPOLLOUT);    
}

void Http::CloseConn(bool real_close){
    printf("Http::CloseConn()\n");
    if(real_close && (SockFd_ != -1)){
         printf("--->Http::CloseConn(), close %d\n", SockFd_);
        removefd(EpollFd, SockFd_);
        SockFd_ = -1;
        UserCount --;
    }
}


sockaddr_in* Http::GetAddress() { 
    return &Address_; 
}

void  Http::CreateSqlCache(SqlPool* connPool){
    printf("Http::CreateSqlCache()\n");
    MYSQL* mysql = nullptr;
    SqlRAII mysqlconn(&mysql,connPool);
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
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

/*
* 以下为关于http请求解析的函数
* 利用状态主状态机和从状态机解析http的请求
* 其中从状态机，也就是解析请求行和头部字段的时候用了regex
*/
Http::LINE_STATUS Http::ReadLine_(){
    cout << "---> Http::ParseLine_()" << endl;
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

char* Http::GetLine_(){
    cout << "---> Http::GetLine()" <<endl;
    return ReadBuf_ + StartLine_;
}

int Http::ParseRequestLine_(std::string &line){
    cout << "---> Http::ParseRequestLine()" << endl;
    cout << "   ---> line = " << line << "size = "<< line.size() << endl;

    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    string method, path, version;
    assert(regex_match(line, subMatch, patten));
    if(regex_match(line, subMatch, patten)) {
        cout << "   --->begin regex" <<endl;  
        method = subMatch[1];
        path = subMatch[2];
        version = subMatch[3];
        cout << method <<"\t"<< path << "\t" << version <<endl;
        if(method == "GET")
            Method_ = GET;
        else if(method == "POST")
            Method_ = POST;
        else
            return 400;
        // 这个地方后面需要优化一下
        Url_ = (char*)malloc(1024);
        memset(Url_,'\0',1024);
        strcpy(Url_, path.c_str());

        if (path == "/"){    
            strcpy(Url_, "/index.html");
        }
        CheckState_ = HEADER;
        cout << "Http::ParseRequestLine_(),CheckState_:REQUESTLINE->HEADER" << endl;
        return 100;
    }
    return 100;
}

int Http::ParseHeader_(char* text){
    cout << "Http::ParseHeader()" << endl;
    if (text[0] == '\0'){
        if (ContentLength_ != 0){
            CheckState_ = CONTENT;
            return 100;
        }
        return 200;
    } else if (strncasecmp(text, "Connection:", 11) == 0){
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0){
            Linger_ = true;
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0){
        text += 15;
        text += strspn(text, " \t");
        ContentLength_ = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0){
        text += 5;
        text += strspn(text, " \t");
        Host_ = text;
    } else {
        cout << "oop!unknow header:" << text << endl;
    }
    return 100;
}
// 只有POST请求会用到
int Http::ParseContent_(char* text){
    cout << "Http::ParseContent()" << endl;
    if (ReadIndex_ >= (ContentLength_ + CheckedIndex_)){
        text[ContentLength_] = '\0';
        //POST请求中最后为输入的用户名和密码
        BodyString_ = text;
        return 200; // 2xx
    }
    return 100; // 1xx接收的请求正在处理
}

int Http::ProcessRequest(){
    cout << "---> Http::ProcessRead()" << endl;
    LINE_STATUS line_status = LINE_OK;
    int ret = 100;
    char *text = 0;
    while (
            (CheckState_ == CONTENT && 
            line_status == LINE_OK) || 
            ((line_status = ReadLine_()) == LINE_OK)
        )
    {
        text = GetLine_();
        StartLine_ = CheckedIndex_;
        std::string line(text,CheckedIndex_);
        switch (CheckState_)
        {
        case REQUESTLINE:
        {
            ret = ParseRequestLine_(line);
            if (ret == 400)
                return 400;
            break;
        }
        case HEADER:
        {
            ret = ParseHeader_(text);
            if (ret == 400)
                return 400;
            else if (ret == 200)
            {
                return FinishParse_();
            }
            break;
        }
        case CONTENT:
        {
            ret = ParseContent_(text);
            if (ret == 200)
                return FinishParse_();
            line_status = LINE_OPEN;
            break;
        }
        default:
            return 500;
        }
    }
    return 100;
}


int Http::FinishParse_(){
    cout << "Http::FinishParse_" << endl;
    cout << "RealFile_ : " << RealFile_ << endl;
    cout << "DocRoot_ : " << DocRoot_ << endl;
    strcpy(RealFile_, DocRoot_);
    int len = strlen(DocRoot_);
    // 找到最后一个的 / 的位置
    const char *p = strrchr(Url_, '/');
    printf("Url_ : %s\n", Url_);
    if ((*(p + 1) == '2' || *(p + 1) == '3')){
        char *url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(url_real, "/");
        strcat(url_real, Url_ + 2);
        strncpy(RealFile_ + len, url_real, REALFILE_LEN - len - 1);
        free(url_real);

        // 将用户名和密码提取出来
        // 比如POST请求实体主体是user=123&passwd=123
        char name[100], password[100];
        int i;
        for (i = 5; BodyString_[i] != '&'; ++i)
            name[i - 5] = BodyString_[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; BodyString_[i] != '\0'; ++i, ++j)
            password[j] = BodyString_[i];
        password[j] = '\0';

        // 3表示注册
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
                int res = mysql_query(HttpMysql, sql_insert);
                Users.insert(pair<string, string>(name, password));
                Lock.unlock();

                if (!res)
                    strcpy(Url_, "/index.html");
                else
                    strcpy(Url_, "/register_error.html");
            }
            else
                strcpy(Url_, "/register_error.html");
        }else if(*(p + 1) == '2'){
            //这时候只需要对哈希表进行判断，不需要再对数据库进行查询
            if (Users.find(name) != Users.end() && Users[name] == password)
                strcpy(Url_, "/jiawang.html");//图片，视频
            else
                strcpy(Url_, "/index_error.html");//登录错误界面    
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
        strcpy(m_url_real, "/index.html");
        strncpy(RealFile_ + len, m_url_real, strlen(m_url_real));
        free(m_url_real);
    }else
        strncpy(RealFile_ + len, Url_, REALFILE_LEN - len - 1);
        printf("not number RealFile_ : %s\n", RealFile_);

    if (stat(RealFile_, &FileStat_) < 0)
        return 404;// 后面要改成404
    if (!(FileStat_.st_mode & S_IROTH))
        return 403;// 后面要改成403
    if (S_ISDIR(FileStat_.st_mode))
        return 400;      

    int fd = open(RealFile_, O_RDONLY);
    FileAddress_ = (char *)mmap(0, FileStat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    //cout << "m_file_address:%s\n" << FileAddress_ << endl;//打印m_file_address
    close(fd);
    return 408;//FILE_REQUEST
}


/*
* 以下为http响应的函数
*
*/
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
const char *error_400_title = "Bad Http";
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


bool Http::ProcessResponse(int response){
    printf("Http::ProcessResponse()\n");
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


void Http::Unmap_(){
    printf("Http::Unmap()\n");
    if (FileAddress_)
    {
        munmap(FileAddress_, FileStat_.st_size);
        FileAddress_ = 0;
    }

}
bool Http::AddResponse_(const char* format,...){
    printf("--->Http::AddResponse_()\n");
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
    //LOG_INFO("request:%s", WriteBuf_);
    //printf("--->--->Http::AddResponse_(),request:%s\n", WriteBuf_);
    return true;
}

bool Http::AddStatusLine_(int status, const char* title){
    printf("Http::AddStatusLine_()\n");
    return AddResponse_("%s %d %s\r\n", "HTTP/1.1", status, title);
}
// AddHeaders()
//     └───AddContentLength()
//     └───AddLinger()
//     └───AddBlankLine()
bool Http::AddHeaders_(int content_length){
    printf("Http::AddHeaders_()\n");
    return AddContentLength_(content_length)
           && AddLinger_()
           && AddBlankLine_();
}
bool Http::AddContentLength_(int content_length){
    printf("Http::AddContentLength_()\n");
    return AddResponse_("Content-Length:%d\r\n", content_length);
}

bool Http::AddLinger_(){
    printf("Http::AddLinger_()\n");
    return AddResponse_("Connection:%s\r\n", 
            (Linger_ == true) ? "keep-alive" : "close");
}
bool Http::AddBlankLine_(){
    printf("Http::AddBlankLine_()\n");
    return AddResponse_("%s", "\r\n");
}


bool Http::AddContentType_(){
    printf("Http::AddContentType_()\n");
    return AddResponse_("Content-Type:%s\r\n", "text/html");
}

bool Http::AddContent_(const char* content){
    printf("Http::AddContent_()\n");
    return AddResponse_("%s", content);
}





