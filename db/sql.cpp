#include "sql.h"




SqlPool::SqlPool():
    MaxConn_(0),
    CurConn_(0)
{
    
}


void SqlPool::init(string url,string user,string passwd,
                string dbName,int port,int maxConn)
{
    

}


MYSQL* SqlPool::GetConnection()
{

}


bool SqlPool::ReleaseConnection(MYSQL *conn)
{

}


int  SqlPool::GetFreeConn()
{

}


void SqlPool::DestoryPool()
{

}

SqlPool* SqlPool::GetInstance()
{

}
