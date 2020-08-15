#include <iostream>
#include <string>
#include <fstream>
#include "rapidjson/prettywriter.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

using namespace std;
using namespace rapidjson;


int main()
{
    ifstream t("document.json"); // 输入流
    IStreamWrapper isw(t);
    Document d;
    d.ParseStream(isw);

    cout << "数据库信息" << endl;
    assert(d.HasMember("database"));
    const Value& dbObject = d["database"];
    if(dbObject.IsObject()){
        if(dbObject.HasMember("username"))
            cout << dbObject["username"].GetString() << endl; 
        if(dbObject.HasMember("password"))
            cout << dbObject["password"].GetString() << endl; 
        if(dbObject.HasMember("dbname"))
            cout << dbObject["dbname"].GetString() << endl; 
        if(dbObject.HasMember("port"))
            cout << dbObject["port"].GetInt() << endl; 
    }


    cout << "数据库信息" << endl;
    assert(d.HasMember("database"));
    const Value& dbObject = d["database"];
    if(dbObject.IsObject()){
        if(dbObject.HasMember("username"))
            cout << dbObject["username"].GetString() << endl; 
        if(dbObject.HasMember("password"))
            cout << dbObject["password"].GetString() << endl; 
        if(dbObject.HasMember("dbname"))
            cout << dbObject["dbname"].GetString() << endl; 
        if(dbObject.HasMember("port"))
            cout << dbObject["port"].GetInt() << endl; 
    }

}