#ifndef __SQL_H__
#define __SQL_H__

#include <cstdio>
#include <list>
#include <mysql/mysql.h>
#include <cstring>
#include <iostream>
#include <string>
#include <pthread.h>
#include "../os/locker.h"
using namespace std;

// 单例数据库，单例指数据库连接池只有一个 
class SqlPool
{
private:
    SqlPool();
    ~SqlPool();
public:
    MYSQL* GetConnection();
    bool ReleaseConnection(MYSQL *conn);
    int  GetFreeConn();
    void DestoryPool();

    static SqlPool* GetInstance();
    void init(string url,string user,string passwd,
                   string dbName,int port,int maxConn);
    
private:
    int MaxConn_;
    int CurConn_;
    int FreeConn_;
    locker Lock_;
    list<MYSQL*> ConnList_;
    sem Reserve_;

public:
    string Url_;
    string Port_;
    string User_;
    string Passwd_;
    string DBName_;
};


// RAII包装
class SqlRAII
{
public:
    SqlRAII(MYSQL** con,SqlPool* connPool);
    ~SqlRAII();

private:
    MYSQL* ConnRAII_;
    SqlPool* PoolRAII_;
};

#endif 