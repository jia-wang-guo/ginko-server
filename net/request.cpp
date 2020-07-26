#include "request.h"


Request::LINE_STATUS Request::ReadLine_(){
    cout << "---> Request::ParseLine()\n" << endl;
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

char* Request::GetLine_(){
    cout << "---> Request::GetLine()\n" <<endl;
    return ReadBuf + StartLine_;
}

int Request::ParseRequestLine_(std::string &line){
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
            return 400;
        // 这个地方后面需要优化一下
        Url_ = (char*)malloc(1024);
        memset(Url_,'\0',1024);
        strcpy(Url_, path.c_str());

        if (path == "/"){    
            strcpy(Url_, "/index.html");
        }
        CheckState_ = HEADER;
        return 100;
    }
    return 100;
}

int Request::ParseHeader_(char* text){
    printf("Request::ParseHeader()\n");
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
        printf("oop!unknow header: %s\n", text);
    }
    return 100;
}
// 只有POST请求会用到
int Request::ParseContent_(char* text){
    printf("Request::ParseContent()\n");
    if (ReadIndex_ >= (ContentLength_ + CheckedIndex_)){
        text[ContentLength_] = '\0';
        //POST请求中最后为输入的用户名和密码
        BodyString_ = text;
        return 200;
    }
    return 100;
}

int Request::ProcessRead(){
    printf("---> Request::ProcessRead()\n");
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


int Request::FinishParse_(){
    printf("Http::DoRequest()\n");
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
        for (i = 5; BodyString_[i] != '&'; ++i)
            name[i - 5] = BodyString_[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; BodyString_[i] != '\0'; ++i, ++j)
            password[j] = BodyString_[i];
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

            if (SqlUser->find(name) == SqlUser->end()){
                Lock_.lock();
                //注册的时候，把数据库和哈希表都锁起来
                //在哈希表和数据库中都插入新的用户名和密码
                int res = mysql_query(HttpRequestMysql, sql_insert);
                SqlUser->insert(pair<string, string>(name, password));
                Lock_.unlock();

                if (!res)
                    strcpy(Url_, "/index.html");
                else
                    strcpy(Url_, "/index.html");
            }
            else
                strcpy(Url_, "/index.html");
        }else if(*(p + 1) == '2'){
            //这时候只需要对哈希表进行判断，不需要再对数据库进行查询
            if (SqlUser->find(name) != SqlUser->end() && (*SqlUser)[name] == password)
                strcpy(Url_, "/index.html");//图片，视频
            else
                strcpy(Url_, "/index.html");//登录错误界面    
        }
    }

    if (*(p + 1) == '0'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/index.html");
        strncpy(RealFile_ + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '1'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/index.html");
        strncpy(RealFile_ + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '5'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/index.html");
        strncpy(RealFile_ + len, m_url_real, strlen(m_url_real));

        free(m_url_real);
    }
    else if (*(p + 1) == '6'){
        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/index.html");
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

    if (stat(RealFile_, &FileStat) < 0)
        return 404;// 后面要改成404
    if (!(FileStat.st_mode & S_IROTH))
        return 403;// 后面要改成403
    if (S_ISDIR(FileStat.st_mode))
        return 400;      

    int fd = open(RealFile_, O_RDONLY);
    FileAddress = (char *)mmap(0, FileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    printf("m_file_address:%s\n",FileAddress);//打印m_file_address
    close(fd);
    return 408;//FILE_REQUEST
}