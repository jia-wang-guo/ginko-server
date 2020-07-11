#include "../sql.h"

#define DEBUG

int main()
{
    // 单例
    SqlPool* pool = SqlPool::GetInstance();
    MYSQL* con = nullptr;
    (*pool).init("localhost",3306,"root","xc242000","jiawangdb",8);
    
    // 取一个连接con
    SqlRAII pool_conn(&con,pool);


    if (mysql_query(con, "SELECT username,passwd FROM user"))
    {
        printf("SELECT error:%s\n", mysql_error(con));
    }

    //从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(con);

    //返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    //返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        cout << row[0] << " " << row[1] <<endl;
    }

    return 0;
}