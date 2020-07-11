/*
* 单例入口数据库连接池，先建立好数据库连接，后面可以继续用
* 连接池构造函数和析构函数被声明为私有，禁止用户调用，只能在成员函数里面调用
*/

#ifndef __SQL_H__
#define __SQL_H__

#include <cstdio>
#include <list>
#include <cassert>
#include <mysql/mysql.h>
#include <cstring>
#include <iostream>
#include <string>
#include <pthread.h>
#include "../os/locker.h"
using namespace std;


class SqlPool
{
// Singleton 
private:
    SqlPool();
    ~SqlPool();
public:
    MYSQL* GetConnection();
    bool ReleaseConnection(MYSQL *conn);
    int  GetFreeConn();
    void DestoryPool();

    static SqlPool* GetInstance();
    void init(string url,int port,string user,
        string passwd,string dbName,int maxConn);
    
private:
    int MaxConn_;
    int CurConn_;
    int FreeConn_;
    locker Lock_;
    list<MYSQL*> ConnList_;
    sem Reserve_;

public:
    string Url_;
    int Port_;
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