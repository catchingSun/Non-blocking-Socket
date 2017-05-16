/*
*作业：
*编写非阻塞模式下套接字应用，
*服务器端可以接受5、50、100个客户端连接请求
*（后台守护，运行过程中和前台交互，
*控制每个线程的流量）。
*
*/

#include "server.h"


fd_set read_fds,old_fds;
int max_fd = -1;
int num;
int old_maxseg;
int old_send_size;
int old_send_lowat;
int send_lowat = 2;
int send_size = 2305; //设置发送缓冲区大小
int maxseg = 8 * 20;

LIST_HEAD(fd_queue);
pthread_mutex_t ACCEPT_LOCK = PTHREAD_MUTEX_INITIALIZER;

static void bail(const char *on_what){
	fputs(strerror(errno),stderr);
	fputs(":",stderr);
	fputs(on_what,stderr);
	fputc('\n',stderr);
	exit(1);
}


int main(){
	int socket_fd;
	int port_number = SERVER_CONNECT_PORT;
	int sr = 1;
	struct sockaddr_in server_addr;
	socklen_t optlen;

	signal(SIGPIPE, sig_pipe);

	if((socket_fd = socket(PF_INET,SOCK_STREAM,0)) == -1){
		fprintf(stderr,"Socket error: %s\a\n",strerror(errno));
		exit(1);
	}
	optlen = sizeof(sr);
	int ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &sr, optlen);
	if(ret){
		bail("setsockopt()  ");
	}
	
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_number);  //转换为网络字节序
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr("10.100.8.49");
	if(bind(socket_fd,(struct sockaddr *)&server_addr,sizeof(struct sockaddr_in)) == -1){
		fprintf(stderr,"bind error:%s \n\a",strerror(errno));
		exit(1);
	}
	
	
	optlen = sizeof(old_maxseg);
	ret = getsockopt(socket_fd, IPPROTO_TCP, TCP_MAXSEG, &old_maxseg, &optlen);
	if(ret){
		printf("获取最大分节大小错误: %d\n", errno);
	}else{
		printf("最大分节大小: %d\n", old_maxseg);
	}

	if((listen(socket_fd,LISTEN_COUNT))){
		fprintf(stderr,"listen error:%s \n\a",strerror(errno));
		exit(1);
	}
	printf("Server started.\n");
	max_fd = socket_fd;
//	control_sockets_by_select(socket_fd);
	control_sockets_by_threads(socket_fd);
	close(socket_fd);
	
	return 0;
}


void sig_pipe(int sign){
	printf("Connect reset by client.\n");
}

int control_sockets_by_threads(int socket_fd){

	int err;
	int i;
	pthread_t ntid[CONN_NUM];
	for(i = 0; i < CONN_NUM; i++){
		
		err = pthread_create(&ntid[i], NULL, thread_function, (void *)&socket_fd);
		if(err != 0){
			printf("Accept errno: \n", errno);
			printf("Create thread failed: %s.\n", strerror(err));
			exit(1);
		}
	}

	for(i = 0; i < CONN_NUM; i++){
		pthread_join(ntid[i], NULL);
	}

//	return 0;
}

void *thread_function(void *arg){
	
	struct sockaddr_in client_addr;
	int new_fd;
	char read_buf[BUFSIZE];
	char write_buf[BUFSIZE];
	int z;
	socklen_t sin_size;
	
	sin_size = sizeof(struct sockaddr);
	pthread_mutex_lock(&ACCEPT_LOCK);
	if((new_fd = accept(*((int *)arg), (struct sockaddr *)(&client_addr), &sin_size)) == -1){
		perror("accept");
		exit(1);
	}
	printf("=====================================\n");
	pthread_mutex_unlock(&ACCEPT_LOCK);
	if((z = read(new_fd,read_buf,sizeof(read_buf))) <= 0){
		if(z < 0 && errno != ECONNRESET){
			bail("read");
		}

	}else{
		printf("%s\n",read_buf );
		/*strcpy(write_buf,"Connect success");
		z = write(new_fd, write_buf, strlen(write_buf));
		if(z < 0){
			bail("write()");
		}*/
		write_rep(new_fd);

	}

}

int control_sockets_by_select(int socket_fd){
	P_SKOPT p_so;
	int n;
	struct timeval time_out;
    time_out.tv_sec = 0;
    time_out.tv_usec = 50000;
    init(socket_fd);
	for(;;){
		read_fds = old_fds;

		n = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
		for(; n > 0; n--){
			list_for_each_entry(p_so, &fd_queue, list){
				if(FD_ISSET(p_so->fd, &read_fds)){
					p_so->do_task(p_so);
				}
			}
		}
		
	}
	return 0;
}

int init(int socket_fd){
	P_SKOPT p_so;
	num = 0;
	if((p_so = (P_SKOPT)malloc (sizeof(SKOPT))) == NULL){
		perror("malloc");
		return -1;
	}
	assert(list_empty(&fd_queue));

	p_so->do_task = create_conn;
	p_so->fd = socket_fd;
	list_add_tail(&p_so->list, &fd_queue);
	num++;
	FD_ZERO(&old_fds);
	FD_SET(socket_fd,&old_fds);
	return 0;
}

int set_send_size(int new_fd,int send_size){
	int optlen;
	int err;
	int change_size;
	struct linger optval_linger;
	optval_linger.l_onoff = 1; //设置延迟关闭生效
	optval_linger.l_linger = 60; //设置延迟关闭时间为60s

	err = setsockopt(new_fd, SOL_SOCKET, SO_LINGER, &optval_linger, sizeof(optval_linger));
	if(err){
		perror("setsockopt()");
	}

	optlen = sizeof(old_send_size);
	//获取原始发送缓冲区大小
	err = getsockopt(new_fd, SOL_SOCKET, SO_SNDBUF, &old_send_size, &optlen);
	if(err){
		printf("获取发送缓冲区大小错误: %d\n", errno);
	}else{
		printf("发送缓冲区原始大小: %d\n", old_send_size);
	}
	optlen = sizeof(send_size);
	//设置发送缓冲区大小
	err = setsockopt(new_fd, SOL_SOCKET, SO_SNDBUF, &send_size, optlen);
	if(err){
		printf("缓冲区大小设置错误: %d\n", errno);
	}else{
		optlen = sizeof(change_size);
		//获取改变后的缓冲区大小
		err = getsockopt(new_fd, SOL_SOCKET, SO_SNDBUF, &change_size, &optlen);
		printf("发送缓冲区大小已设置为：%d\n", change_size);
	}

	optlen = sizeof(old_maxseg);
	err = getsockopt(new_fd, IPPROTO_TCP, TCP_MAXSEG, &old_maxseg, &optlen);
	if(err){
		printf("获取最大分节大小错误: %d\n", errno);
	}else{
		printf("最大分节大小: %d\n", old_maxseg);
	}
/*	optlen = sizeof(maxseg);
	//设置发送缓冲区下限
	err = setsockopt(new_fd, IPPROTO_TCP, TCP_MAXSEG, &maxseg, optlen);
	if(err){
		printf("最大分节大小设置错误: %d\n", errno);
	}else{
		optlen = sizeof(maxseg);
		//获取改变后的缓冲区下限
		err = getsockopt(new_fd, IPPROTO_TCP, TCP_MAXSEG, &maxseg, &optlen);
		printf("最大分节大小已设置为：%d\n", maxseg);
	}

*/

}


int create_conn(struct sockfd_opt *p_so){
	struct sockaddr_in client_addr;
	int new_fd;
	int ret;
	int optlen;
	socklen_t sin_size;
	
	sin_size = sizeof(struct sockaddr_in);
	printf("===================Wait for client connect==================\n");
	if((new_fd = accept(p_so->fd, (struct sockaddr *)(&client_addr), &sin_size)) == -1){
		fprintf(stderr,"accept error:%s \n\a",strerror(errno));
		exit(1);
	}
	if(send_size < old_send_size){
		send_size += 64;
	}
	//maxseg += 56;
	
	set_send_size(new_fd, send_size);


	printf("new_fd: %d \n", new_fd );
	if(num++ == FD_SETSIZE){
		printf("Too many clients");
		close(new_fd);
		return -1;
	}
	if((p_so = (P_SKOPT)malloc (sizeof(SKOPT))) == NULL){
		perror("0");
		return -1;
	}
	p_so->fd = new_fd;
	p_so->do_task = send_reply;
	list_add_tail(&p_so->list, &fd_queue);
	FD_SET(new_fd, &old_fds);
	if(new_fd > max_fd){
		max_fd = new_fd;
	}
	return 0;
}


int send_reply(struct sockfd_opt *p_so){
	char read_buf[BUFSIZE];
	int z = 1;
	
	memset(read_buf, 0, sizeof(read_buf));
	if((z = read(p_so->fd,read_buf,sizeof(read_buf))) <= 0){
		printf("Read errno : %d\n",errno);
		if(errno != EWOULDBLOCK && errno != EINTR && errno != EAGAIN){
			shutdown(p_so->fd, 2);
			FD_CLR(p_so->fd, &old_fds);
			list_del(&p_so->list);
			free(p_so);
		}
	/*	else if(z == 0){ 
			shutdown(p_so->fd, 2);
			FD_CLR(p_so->fd, &old_fds);
			list_del(&p_so->list);
			free(p_so);
		}*/
	}else{
		printf("Read from client:%s\n",read_buf);

		write_rep(p_so->fd);
	}
	return 0;
}

int write_rep(int socket_fd){
	char filename[] = FILE_NAME;
	ssize_t size = -1;
	int z = 1;
	int file_fd;
	int flags;
	int file_size = 0;
	char write_buf[BUFSIZE];
	file_fd = open(filename, O_RDONLY);
		if(file_fd == -1){
			printf("Open %s failure!,file_fd = %d\n",filename,file_fd);

		}else{
			z = 1;
			while(z != 0 && size != 0){
				z = 0;
				memset(write_buf, 0, sizeof(write_buf));
				size = read(file_fd, write_buf, sizeof(write_buf));
			//	printf("\nwrite buf size: %d\n", strlen(write_buf));
			//	printf("\n size = %d\n",size);
				if(size > 0){
					if((flags = fcntl(socket_fd,F_GETFL,0) ) < 0){
						bail("f_cntl(F_GETFL)");
					}
					flags |= O_NONBLOCK;
					if(fcntl(socket_fd,F_SETFL,flags) < 0){
						bail("f_cntl(F_GETFL)");
					}
					z = write(socket_fd, write_buf, size);
					if(z < 0){
						printf("Write errno : %d\n",errno);
						while(z < 0 && errno == EAGAIN)  
           				{  
               		 		usleep(1000);  
               		 		z = write(socket_fd, write_buf, size);
            			}  
					}
					else{
						file_size += z;
					}
				//	printf("\nz = %d\n", z);
				}
			}
			printf("\nWrite %d end.\n", file_size);
			close(file_fd);
		}
}