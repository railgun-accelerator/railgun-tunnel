/*
 * railgun_common.h
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#ifndef RAILGUN_COMMON_H_
#define RAILGUN_COMMON_H_

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
#include <assert.h>
#include <time.h>

#include "railgun_typedef.h"

#define MAXBUF 2048
#define MAXSERVEREPOLLSIZE 100
#define MAXCLIENTEPOLLSIZE 10

#define SERV_PORT 41234

#define MIN_DELAY_ACK (u_int16_t)100

#define MAX_DELAY_ACK (u_int16_t)200

#define RTO (u_int16_t)500

#define ISN (u_int8_t)0

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#ifndef BOOL
#define BOOL u_int16_t
#define FALSE (u_int16_t) 0
#define TRUE (u_int16_t) 1
#endif

extern void railgun_timer_delete();

extern void railgun_timer_cancel();

extern void railgun_timer_set(int time_in_ms);

extern void railgun_timer_init(void (*timer_handler)(int sig, siginfo_t *si, void *uc), void* param);

extern int railgun_packet_write(RAILGUN_HEADER* packet, int fd, u_int8_t *buffer,
		u_int8_t *payload, int *plength);

extern int railgun_packet_read(u_int8_t *buffer);

#endif /*RAILGUN_COMMON_H_*/
