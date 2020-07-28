#include "../db/sql.h"
#include "../net/http.h"
#include "../os/timer.h"
#include "../os/locker.h"
#include "../os/thread.h"
#include "ginko.h"
#include <fstream>
#include <cassert>
int main()
{
    // std::ifstream confStream("../etc/ginko.conf");
    // assert(confStream.is_open());

    // string user;
    // confStream >> user;
    // string passwd;
    // confStream >> passwd;
    // string dbname;
    // confStream >> dbname;
    // int port;
    // confStream >> port;

    //cout << port << endl;



    Ginko ginko;
    ginko.init(8001, "root", "xc242000", "jiawangdb",1,8,8);
    ginko.SqlPoolInit();
    ginko.ThreadPoolInit();
    ginko.EventListen();
    ginko.EventLoop();        

    return 0;
}