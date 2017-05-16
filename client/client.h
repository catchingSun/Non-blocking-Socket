#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>

#define SERVER_IP "192.168.61.138"
#define SERVER_PORT 50000
#define BUFSIZE 512

void sig_proccess(int sign);
void sig_pipe(int sign);
