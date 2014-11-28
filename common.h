#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <assert.h>


#define MAXBUF 1024
#define MAXEPOLLSIZE 100

#define SERV_PORT 41234

#define CLIENT_THREAD_COUNT 1


#endif /*COMMON_H_*/
