/*
 * railgun_resp_queue.h
 *
 *  Created on: 2014年12月3日
 *      Author: mabin
 */

#ifndef RAILGUN_RESP_QUEUE_H_
#define RAILGUN_RESP_QUEUE_H_

#include <railgun_common.h>

extern RESP_HEADER resp_head;

#define for_packet_in_resp_queue(s) list_for_each_prev_entry(s, &resp_head.head, head)

#define is_resp_queue_empty list_empty(&(resp_head.head))

static inline RESP_HEADER* resp_queue_begin() {
	return list_entry(resp_head.head.prev, RESP_HEADER, head);
}

static inline RESP_HEADER* resp_queue_add_allocate(int ack, int seq, struct sockaddr_in* addr_buffer, int length) {
	RESP_HEADER* packet = (RESP_HEADER*) malloc(sizeof(RESP_HEADER));
	bzero(packet, sizeof(RAILGUN_HEADER));
	packet->ack = ack;
	packet->seq = seq;
	memcpy(&packet->addr, addr_buffer, sizeof(struct sockaddr_in));
	_list_add(&packet->head, &resp_head.head);
	return packet;
}

static inline RESP_HEADER* resp_queue_add(RESP_HEADER* packet) {
	_list_add(&packet->head, &resp_head.head);
	return packet;
}

static inline void resp_queue_delete(RESP_HEADER* packet) {
	list_del(&packet->head);
	free(packet);
}

#endif /* RAILGUN_RESP_QUEUE_H_ */
