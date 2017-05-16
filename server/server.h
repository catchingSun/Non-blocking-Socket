#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h> 
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>


#include "list.h"

#define CONN_NUM 500
#define SERVER_CONNECT_PORT 50000
#define BUFSIZE 1024
#define LISTEN_COUNT 128
#define FILE_NAME "./homework.rar"


struct sockfd_opt{
	int fd;
	int (*do_task)(struct sockfd_opt *p_so);
	struct list_head list;
};

struct thread_function_args{
	int sockfd_fd;
	
};

typedef struct sockfd_opt SKOPT,*P_SKOPT;

void sig_pipe(int sign);
int init(int socket_fd);
int control_sockets_by_threads(int socket_fd);
int control_sockets_by_select(int socket_fd);
int set_send_size(int new_fd,int send_size);
int send_reply(struct sockfd_opt *p_so);
int create_conn(struct sockfd_opt *p_so);
void *thread_function(void *arg);
int write_rep(int socket_fd);
