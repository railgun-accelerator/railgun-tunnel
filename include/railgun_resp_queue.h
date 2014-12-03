/*
 * railgun_resp_queue.h
 *
 *  Created on: 2014年12月3日
 *      Author: mabin
 */

#ifndef RAILGUN_RESP_QUEUE_H_
#define RAILGUN_RESP_QUEUE_H_

#include <railgun_common.h>

extern PAYOAD_RESP resp_head;

#define for_packet_in_resp_queue(s) list_for_each_prev_entry(s, &resp_head.head, head)

#define is_resp_queue_empty list_empty(&(resp_head.head))

static inline PAYOAD_RESP* resp_queue_begin() {
	return list_entry(resp_head.head.prev, PAYOAD_RESP, head);
}

static inline PAYOAD_RESP* resp_queue_add(int ack, struct sockaddr_in* addr_buffer) {
	PAYOAD_RESP* packet = (PAYOAD_RESP*) malloc(sizeof(PAYOAD_RESP));
	bzero(packet, sizeof(RAILGUN_PACKET));
	packet->response1 = ack;
	packet->response2 = ack;
	memcpy(&packet->addr, addr_buffer, sizeof(struct sockaddr_in));
	_list_add(&packet->head, &resp_head.head);
	return packet;
}

static inline void resp_queue_delete(PAYOAD_RESP* packet) {
	list_del(&packet->head);
	free(packet);
}

#endif /* RAILGUN_RESP_QUEUE_H_ */
