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
#include "railgun_log.h"

#define MAX_DELIVER_SACK 16

#define BUFFER (u_int32_t)1024 * 1024 * 10

#define MAXBUF 2048
#define MAXSERVEREPOLLSIZE 100
#define MAXCLIENTEPOLLSIZE 10

#define MTU (u_int16_t)1500 - 20 - 8

#define SERV_PORT 41234

#define ATO (u_int16_t)200

#define RTO (u_int16_t)500

#define ISN (u_int8_t)0

extern void railgun_timer_delete();

extern void railgun_timer_cancel();

extern void railgun_timer_set(int time_in_ms);

extern void railgun_timer_init(
		void (*timer_handler)(int sig, siginfo_t *si, void *uc), void* param);

extern int railgun_packet_write(RAILGUN_HEADER* packet, int fd,
		u_int8_t *buffer, u_int8_t *payload, int *plength);

extern int railgun_tcp_read(int fd, u_int8_t *buffer, int start_pos, int *pmax_avail_size);

extern int railgun_packet_read(int fd, u_int8_t *buffer, RAILGUN_HEADER *pheader);

extern void railgun_resp_allocate(u_int8_t *buffer, RESP_HEADER *pheader,
		int *payload_pos, int ack_allocated);

extern void railgun_resp_send(RESP_HEADER* resp, int fd, u_int8_t *buffer);

extern int railgun_resp_read(int fd, u_int8_t *buffer, RESP_HEADER *pheader);

#endif /*RAILGUN_COMMON_H_*/
