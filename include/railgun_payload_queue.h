/*
 * railgun_payload_queue.h
 *
 *  Created on: 2014年12月3日
 *      Author: mabin
 */
#ifndef RAILGUN_PAYLOAD_QUEUE_H_
#define RAILGUN_PAYLOAD_QUEUE_H_

#include <railgun_common.h>

extern RAILGUN_PACKET payload_head;

#define for_packet_in_payload_queue(s, n) list_for_each_prev_entry_safe(s, n, &payload_head.head, head)

#define is_payload_queue_empty list_empty(&(payload_head.head))

static inline RAILGUN_PACKET* payload_queue_begin() {
	return list_entry(payload_head.head.prev, RAILGUN_PACKET, head);
}

static inline RAILGUN_PACKET* payload_queue_add(int seq, u_int8_t * buffer,
		u_int32_t size, u_int64_t time) {
	RAILGUN_PACKET* packet = (RAILGUN_PACKET*) malloc(sizeof(RAILGUN_PACKET));
	bzero(packet, sizeof(RAILGUN_PACKET));
	packet->seq = seq;
	packet->ack = 1;
	packet->timestamp = time;
	memcpy(packet->data, buffer, size);
	_list_add(&packet->head, &payload_head.head);
	return packet;
}

static inline RAILGUN_PACKET* payload_queue_find(int seq) {
	RAILGUN_PACKET *p = NULL, *tmp = NULL;
	BOOL is_found = FALSE;
	for_packet_in_payload_queue(p, tmp) {
		if (p->seq == seq) {
			is_found = TRUE;
			break;
		}
	}
	if (!is_found) {
		p = NULL;
	}
	return p;
}

static inline void payload_queue_delete(RAILGUN_PACKET* packet) {
	list_del(&packet->head);
	free(packet);
}

#endif /*RAILGUN_PAYLOAD_QUEUE_H_*/
