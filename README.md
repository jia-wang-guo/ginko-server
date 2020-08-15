
## 0、环境
- 操作系统：
Ubuntu 20.04 LTS
- Mysql环境：
Mysql 5.7.30
- Json库：rapidjson v1.1.0
## 1、事件处理
本项目主要使用的是同步I/O模拟Proactor模式的事件处理方式，是基于reactor + threadpool的改模型，具体介绍为[事件处理](./doc/event.md)
## 2、线程池
为了减少动态创建和销毁的线程的开销，预先开辟了一个线程池，建立一个工作队列利用信号量让主线程和工作线程进程同步，[线程池介绍](./doc/thread.md)

## 3、HTTP解析
使用状态机解析，其中解析请求行和头部字段使用C++正则表达式
## 4、Json
使用[rapidjson](http://rapidjson.org/zh-cn/)解析配置文件，具体方式，[解析Json配置文件](./doc/json.md)
## 5、定时器
采用最简单的升序链表定时器，《Linux高性能服务器编程》主要介绍了三种定时器，对三种定时器的理解和分析为：[三种定时器分析](./doc/timer.md)
## 6、性能测试
TO DO
## 7、常见问题

## 参考文献
[1] Alexander Libman,Vladimir Gilbourd,Comparing Two High-Performance I/O Design Patterns[OL].2005-11-25.https://www.artima.com/articles/io_design_patterns.html

[2] 游双，Linux高性能服务器编程[M]。机械工业出版社，2013。

[3] 陈硕， Linux多线程服务端编程：使用muduo C++网络库。电子工业出版社，2012。

[4] UNP、APUE、CSAPP


