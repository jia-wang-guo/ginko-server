#include "sql.h"

#define DEBUG


SqlPool::SqlPool():
    CurConn_(0),
    FreeConn_(0)
{
    
}
void SqlPool::init(string url,int port,string user,
        string passwd,string dbName,int maxConn)
{
    Url_     = url;
    Port_    = port;
    User_    = user;
    Passwd_  = passwd;
    DBName_  = dbName;
  
    for(int i = 0; i < maxConn; i++)
    {
        MYSQL* con = nullptr;
        con = mysql_init(con);
        assert(con != nullptr);
        con = mysql_real_connect(con, Url_.c_str(), User_.c_str(), 
                Passwd_.c_str(),DBName_.c_str(), Port_, NULL, 0);
        assert(con != nullptr);
        ConnList_.push_back(con);
        FreeConn_++;
    }
    // 初始化信号量
    Reserve_ = sem(FreeConn_);
    MaxConn_ = FreeConn_;
}


MYSQL* SqlPool::GetConnection()
{
    MYSQL* con = nullptr;
    if(ConnList_.empty())
        return nullptr;
    
    Reserve_.wait(); // P操作
    Lock_.lock();
    con = ConnList_.front();
    ConnList_.pop_front();
    ++FreeConn_;
    ++CurConn_;
    Lock_.unlock();
    return con;
}


bool SqlPool::ReleaseConnection(MYSQL *con)
{
    if(con == nullptr)
        return false;
    Lock_.lock();
    ConnList_.push_back(con);
    ++FreeConn_;
    ++CurConn_;
    Lock_.unlock();
    Reserve_.post(); // V操作
    return true;
}


int  SqlPool::GetFreeConn()
{
    return FreeConn_;
}


void SqlPool::DestoryPool()
{
    Lock_.lock();
    if(ConnList_.size() > 0)
    {
        for(auto con:ConnList_)
        {
            mysql_close(con);
        }
        CurConn_  = 0;
        FreeConn_ = 0;
        ConnList_.clear();
    }
    Lock_.unlock();
}

//
SqlPool* SqlPool::GetInstance()
{
    // effective C++ 04,local static
    static SqlPool connPool;
    return &connPool;
}

SqlPool::~SqlPool()
{
    DestoryPool();
}


SqlRAII::SqlRAII(MYSQL** con,SqlPool* connPool)
{
    *con = connPool->GetConnection();
    ConnRAII_ = *con;
    PoolRAII_ = connPool;

    #ifdef DEBUG
        printf("取出一个连接\n");
    #endif
}
SqlRAII::~SqlRAII()
{
    PoolRAII_->ReleaseConnection(ConnRAII_);

    #ifdef DEBUG
        printf("销毁一个连接\n");
    #endif
}
