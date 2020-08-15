#include <fstream>
#include <cassert>
#include "../db/sql.h"
#include "../os/timer.h"
#include "../os/locker.h"
#include "../os/thread.h"
#include "ginko.h"
#include "../json/rapidjson/prettywriter.h"
#include "../json/rapidjson/document.h"
#include "../json/rapidjson/istreamwrapper.h"

using namespace std;
using namespace rapidjson;

int main()
{
    // 如果json文件没有配置，会使用默认值
    int port = 8001;
    int threadnum = 8;
    string username = "root";
    string password = "root";
    string dbname = "root";
    int sqlnum = 8;
    int opt_linger = 1;

    // json配置文件的位置
    ifstream t("../etc/conf.json"); 
    IStreamWrapper isw(t);
    Document d;
    d.ParseStream(isw);

    assert(d.HasMember("threadpool"));
    const Value& threadObject = d["threadpool"];
    if(threadObject.IsObject()){
        if(threadObject.HasMember("port"))
            port = threadObject["port"].GetInt();
        if(threadObject.HasMember("threadnum"))
            threadnum = threadObject["threadnum"].GetInt();
        if(threadObject.HasMember("opt_linger"))
            opt_linger = threadObject["opt_linger"].GetInt();               
    }

    assert(d.HasMember("database"));
    const Value& dbObject = d["database"];
    if(dbObject.IsObject()){
        if(dbObject.HasMember("username"))
            username =  dbObject["username"].GetString(); 
        if(dbObject.HasMember("password"))
            password =  dbObject["password"].GetString(); 
        if(dbObject.HasMember("dbname"))
            dbname = dbObject["dbname"].GetString(); 
        if(dbObject.HasMember("sqlnum"))
            sqlnum = dbObject["sqlnum"].GetInt(); 
    }

    Ginko ginko;
    ginko.Init(port, username, password, dbname,opt_linger,sqlnum,threadnum);
    ginko.SqlPoolInit();
    ginko.ThreadPoolInit();
    ginko.EventListen();
    ginko.EventLoop();        

    return 0;
}