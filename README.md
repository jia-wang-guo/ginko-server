
## 环境
- 操作系统：
Ubuntu 20.04 LTS
- Mysql环境：
Mysql 5.7.30
## 事件处理
本项目主要使用的是同步I/O模拟Proactor模式的事件处理方式，主线程负责数据读写操作，读写完成后，主线程向

## I/O探赜
本项目用到的I/O主要包括

## 参考文献
[1] Alexander Libman,Vladimir Gilbourd,Comparing Two High-Performance I/O Design Patterns[OL].2005-11-25.https://www.artima.com/articles/io_design_patterns.html

[2] 游双，Linux高性能服务器编程[M]。机械工业出版社，2013。

[3] 陈硕， Linux多线程服务端编程：使用muduo C++网络库。电子工业出版社，2012。

