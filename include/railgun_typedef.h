/*
 * railgun_typedef.h
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#ifndef RAILGUN_TYPE_DEF_H_
#define RAILGUN_TYPE_DEF_H_

#include <railgun_list.h>

#define PAYLOAD_SIZE 1464

typedef struct railgun_packet {
	list_head head;
    u_int64_t timestamp;
    u_int32_t seq;
    u_int32_t ack;
    u_int8_t data[PAYLOAD_SIZE];
} RAILGUN_PACKET;

typedef struct payload_packet {
    u_int32_t seq;
    u_int32_t ack;
    u_int8_t data[PAYLOAD_SIZE];
} PAYLOAD_PACKET;

typedef struct payload_resp {
    u_int32_t response1;
    u_int32_t response2;
    struct sockaddr_in addr;
    list_head head;
} PAYOAD_RESP;

#endif /*RAILGUN_TYPE_DEF_H_*/
