#include <iostream>
#include <regex>
using namespace std;

int main(){
    string line = "GET / HTTP/1.1";
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch subMatch;
    string method, path, version;
    if(regex_match(line, subMatch, patten)) {
        method = subMatch[1];
        path = subMatch[2];
        version = subMatch[3];
        cout << method <<"\t"<< path << "\t" << version <<endl;
    }
    return 0;
}


