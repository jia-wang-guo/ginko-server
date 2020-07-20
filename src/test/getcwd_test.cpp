#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;

int main()
{
    vector<int> a;
    sort(a.begin(),a.end());
    char pwd[200];
    cout << getcwd(pwd,200) << endl;
    return 0;
}
