# Json解析
使用了rapidjson库进行解析，配置文件放在了工程主目录etc文件夹下，[conf.json](../etc/conf.json)
```json
{
   "threadpool":{
       "port" : 8001,
       "threadnum" : 8,
       "opt_linger" : 1
   },

   "database":{
       "username": "root",
       "password": "xc242000",
       "dbname":   "jiawangdb",
       "sqlnum" :  8
   }
}
```
json文件中有两个对象threadpool和database，通过读取连两个对象里面的数字和字符串，可以得到服务器所需要的参数。

```C++
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
```
程序每次启动时会读取conf.json，并解析，把json文件设置相关参数赋值给相应的变量。如果没有在conf.json文件指定参数，程序会使用默认参数。

注：重新配置conf.json文件之后，需要重启服务器才能生效！