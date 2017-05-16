#include "client.h"

static void bail(const char *on_what){
	fputs(strerror(errno),stderr);
	fputs(":",stderr);
	fputs(on_what,stderr);
	fputc('\n',stderr);
	exit(1);
}

int socket_fd;

int main()
{
	
	char read_buf[BUFSIZE];
	char write_buf[BUFSIZE];
	struct sockaddr_in server_addr;
	struct hostent *host;
	int port = SERVER_PORT;
	int z;
	int flags;
	int file_size = 0;
	char *ip = SERVER_IP;
	host = gethostbyname(ip);

	signal(SIGINT, sig_proccess);

	signal(SIGPIPE, sig_pipe);

	if((socket_fd = socket(PF_INET,SOCK_STREAM,0)) == -1){
		printf("create socket error");
	}
	
	memset(&server_addr,0,sizeof server_addr);
	server_addr.sin_family = PF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);

	if(connect(socket_fd,(struct sockaddr *)(&server_addr),sizeof server_addr) == -1){
		fprintf(stderr,"connect error:%s \n\a",strerror(errno));
		exit(1);
	}
	strcpy(write_buf,"Connect");
	
	z = write(socket_fd, write_buf, strlen(write_buf));
	if(z < 0){
		bail("write()");
	}else{
		printf("%s\n",write_buf);
		z = 1;
		while(z != 0){
			memset(read_buf, 0, sizeof(read_buf));
			z = read(socket_fd,read_buf,sizeof(read_buf));
			
			if(z < 0){
				if(errno != EWOULDBLOCK && errno != EAGAIN && errno != EINTR){
					printf("\nz = %d\n", z);
					printf("\nRead errno: %d\n", errno);
					bail("read()");
				}
				
			}else{
				file_size += z;
				printf("\n======%d=========\n", file_size);
				
			}
			
		}


	}
	printf("\nread end\n");
	shutdown(socket_fd,2);
	return 0;
}


void sig_proccess(int sign){
	printf("\nClient exited by Ctrl-C.\n");
	//shutdown(socket_fd,2);
}


void sig_pipe(int sign){
	printf("\nServer exited by SIGPIPE.\n");
}