/*
* http.cpp/Http::ParseLine()函数的逻辑测试
* 从状态机三种情况下的三种输出
*/
#include <iostream>
#include <cstring>
using namespace std;

char m_read_buf[1024];

enum LINE_STATUS
{
    LINE_OK = 0,
    LINE_BAD,
    LINE_OPEN
};

LINE_STATUS parse_line(char* m_read_buf,int m_checked_idx, int m_read_idx){
    char temp;
    for (; m_checked_idx < m_read_idx; ++m_checked_idx)
    {
        temp = m_read_buf[m_checked_idx];
       // cout << temp << endl;
        if (temp == '\r')
        {
            if ((m_checked_idx + 1) == m_read_idx)
                return LINE_OPEN;
            else if (m_read_buf[m_checked_idx + 1] == '\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD; 
        }
        else if (temp == '\n')
        {
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// 没有读取到一行,输出LINE_OPEN(2)
void test01(){
    memset(m_read_buf,'\0',1024);
    const char* temp = "HOST: localhost\r\nConnect";
    memcpy(m_read_buf,temp,strlen(temp));
    cout << strlen(temp) << endl;
    int m_checked_idx = 0;
    int m_read_idx = 3;     //没有读取到一行输出LINE_OPEN
    cout << parse_line(m_read_buf, m_checked_idx, m_read_idx) << endl;
}

// 读取到了一行，输出LINE_OK(0)
void test02(){
    memset(m_read_buf,'\0',1024);
    const char* temp = "HOST: localhost\r\nConnect";
    memcpy(m_read_buf,temp,strlen(temp));
    cout << strlen(temp) << endl;
    int m_checked_idx = 0;
    int m_read_idx = 20;     //没有读取到一行输出LINE_OPEN
    cout << parse_line(m_read_buf, m_checked_idx, m_read_idx) << endl;
}

// 字符串格式错误，输出LINE_BAD(1)
void test03(){
    memset(m_read_buf,'\0',1024);
    const char* temp = "HOST: localhost\rConnect";
    memcpy(m_read_buf,temp,strlen(temp));
    cout << strlen(temp) << endl;
    int m_checked_idx = 0;
    int m_read_idx = 20;     //没有读取到一行输出LINE_OPEN
    cout << parse_line(m_read_buf, m_checked_idx, m_read_idx) << endl;
}



int main()
{
    test01();
    test02();
    test03();
    return 0;
}