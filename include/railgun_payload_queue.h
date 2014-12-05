/*
 * railgun_payload_queue.h
 *
 *  Created on: 2014年12月3日
 *      Author: mabin
 */
#ifndef RAILGUN_PAYLOAD_QUEUE_H_
#define RAILGUN_PAYLOAD_QUEUE_H_

#include <railgun_common.h>

extern RAILGUN_HEADER payload_head;

#define for_packet_in_payload_queue(s, n) list_for_each_prev_entry_safe(s, n, &payload_head.head, head)

#define is_payload_queue_empty list_empty(&(payload_head.head))

static inline RAILGUN_HEADER* payload_queue_begin() {
	return list_entry(payload_head.head.prev, RAILGUN_HEADER, head);
}

static inline RAILGUN_HEADER* payload_queue_add(int seq, int ack,
		u_int32_t offset, u_int64_t time) {
	RAILGUN_HEADER* packet = (RAILGUN_HEADER*) malloc(sizeof(RAILGUN_HEADER));
	bzero(packet, sizeof(RAILGUN_HEADER));
	packet->seq = seq;
	packet->ack = 1;
	packet->timestamp = time;
	packet->src = 0;
	packet->sack_cnt = 0;
	INIT_LIST_HEAD(&packet->sack_head);
	packet->data_offset = offset;
	_list_add(&packet->head, &payload_head.head);
	return packet;
}

static inline RAILGUN_HEADER* payload_queue_find(int seq) {
	RAILGUN_HEADER *p = NULL, *tmp = NULL;
	BOOL is_found = FALSE;
	for_packet_in_payload_queue(p, tmp)
	{
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

static inline void payload_queue_delete(RAILGUN_HEADER* packet) {
	list_del(&packet->head);
	free(packet);
}

#endif /*RAILGUN_PAYLOAD_QUEUE_H_*/
