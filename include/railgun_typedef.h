/*
 * railgun_typedef.h
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#ifndef RAILGUN_TYPE_DEF_H_
#define RAILGUN_TYPE_DEF_H_

#include <railgun_list.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#ifndef BOOL
#define BOOL u_int16_t
#define FALSE (u_int16_t) 0
#define TRUE (u_int16_t) 1
#endif

typedef struct sack_packet {
	u_int32_t left_edge;
	u_int32_t right_edge;
	list_head head;
} SACK_PACKET;

typedef struct railgun_header {
	struct sockaddr_in addr;
	size_t addr_len;
	list_head head;
	u_int64_t timestamp;
	u_int16_t src;
	union {
		u_int32_t _data_offset;
		void* _data;
	} __src_represent;
#define railgun_data_offset __src_represent._data_offset
#define railgun_data __src_represent._data
	u_int32_t railgun_data_length;
	u_int32_t seq;
	u_int32_t ack;
	u_int32_t win;
	u_int32_t sack_cnt;
	list_head sack_head;
} RAILGUN_HEADER;

typedef struct payload_header {
	u_int32_t seq;
	u_int32_t ack;
	u_int32_t win;
	u_int32_t sack_cnt;
	list_head sack_head;
} PAYLOAD_HEADER;

typedef struct resp_header {
	struct sockaddr_in addr;
	size_t addr_len;
	list_head head;
	u_int32_t seq;
	u_int32_t ack;
	u_int32_t sack_cnt;
	list_head sack_head;
} RESP_HEADER;

#endif /*RAILGUN_TYPE_DEF_H_*/
