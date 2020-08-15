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
    int n;
    int score[3];
    double aveScore[3];
    string strSubject[3];
    string wsValue[3];

    ifstream t("document.json"); // 输入流
    IStreamWrapper isw(t);
    Document d;
    d.ParseStream(isw);

    // 获取最简单的单个键的值
    assert(d.HasMember("num"));
    assert(d["num"].IsInt());
    n = d["num"].GetInt();
    cout << "获取num的值 : " << n << endl;

    // 获取数组的值,使用引用来连续访问
    cout << "获取subject" << endl;
    assert(d.HasMember("subject"));
    const Value& subjectArray = d["subject"];
    if (subjectArray.IsArray())
    {
        for (int i = 0; i < subjectArray.Size(); i++)
        {
            strSubject[i] = subjectArray[i].GetString();
            cout << strSubject[i] << endl;
        }
    }


    // 获取对象中的数组，也就是对象是一个数组
    cout << "获取info" << endl;
    assert(d.HasMember("info"));
    const Value& infoArray = d["info"];
    if (infoArray.IsArray()) {
        for (int i = 0; i < infoArray.Size(); i++) {
            const Value& tempInfo = infoArray[i];

            string strValue = tempInfo["value"].GetString();
            wsValue[i] = strValue;

            score[i] = tempInfo["score"].GetInt();
            aveScore[i] = tempInfo["aveScore"].GetDouble();
            cout <<  wsValue[i] << " " << score[i] << " " <<  aveScore[i] << endl;
        }
    }


}