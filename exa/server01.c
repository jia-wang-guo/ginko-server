// UNP30.5 TCP并发服务器程序，每个客户一个子进程 
#include <sys/resource.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <error.h>
#include <signal.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>

void pr_cpu_time(void);

void sig_int(int signo)
{
	pr_cpu_time();
	exit(0);
}

void sig_chld(int signo)
{
	pid_t	pid;
	int		stat;

	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		printf("child %d terminated\n", pid);
	}
	return;
}


void pr_cpu_time(void)
{
	double			user, sys;
	struct rusage	myusage, childusage;

	if (getrusage(RUSAGE_SELF, &myusage) < 0)
		err_sys("getrusage error");
	if (getrusage(RUSAGE_CHILDREN, &childusage) < 0)
		err_sys("getrusage error");

	user = (double) myusage.ru_utime.tv_sec +
					myusage.ru_utime.tv_usec/1000000.0;
	user += (double) childusage.ru_utime.tv_sec +
					 childusage.ru_utime.tv_usec/1000000.0;
	sys = (double) myusage.ru_stime.tv_sec +
				   myusage.ru_stime.tv_usec/1000000.0;
	sys += (double) childusage.ru_stime.tv_sec +
					childusage.ru_stime.tv_usec/1000000.0;

	printf("\nuser time = %g, sys time = %g\n", user, sys);
}

int main(int argc, char **argv)
{
    if(argc < 2){
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
	int					listenfd, connfd;
	pid_t				childpid;
	socklen_t			clilen, addrlen;
    struct sockaddr_in  cliaddr, servaddr;
    // socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &servaddr.sin_addr);
    // bind
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    // listen
    listen(listenfd, 1024);

    // signal相关
	assert(signal(SIGCHLD, sig_chld) != SIG_ERR);
    assert(signal(SIGINT, sig_int) != SIG_ERR);


	for ( ; ; ) {
		clilen = addrlen;
		if ( (connfd = accept(listenfd, cliaddr, &clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				err_sys("accept error");
		}

		if ( (childpid = Fork()) == 0) {	/* child process */
			Close(listenfd);	/* close listening socket */
			web_child(connfd);	/* process request */
			exit(0);
		}
		Close(connfd);			/* parent closes connected socket */
	}
}
