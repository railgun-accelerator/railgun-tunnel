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

static inline RESP_HEADER* resp_queue_add_allocate(int ack, int seq,
		struct sockaddr_in* paddr_buffer, int length, int size,
		SACK_PACKET* psack_queue_head) {
	SACK_PACKET* sack_p = NULL;
	RESP_HEADER* packet = (RESP_HEADER*) malloc(sizeof(RESP_HEADER));
	bzero(packet, sizeof(RESP_HEADER));
	packet->ack = ack;
	packet->seq = seq;
	packet->sack_cnt = size > MAX_DELIVER_SACK ? MAX_DELIVER_SACK : size;
	packet->addr_len = length;
	INIT_LIST_HEAD(&packet->sack_head);
	memcpy(&packet->addr, paddr_buffer, packet->addr_len);
	int i = 0;
	printf("before copy size = %d \n", packet->sack_cnt);
	if (packet->sack_cnt != 0) {
		sack_p = (SACK_PACKET*) calloc(packet->sack_cnt, sizeof(SACK_PACKET));
	}
	SACK_PACKET *iter_sack = NULL;
	list_for_each_prev_entry(iter_sack, &psack_queue_head->head, head)
	{
		if (i >= packet->sack_cnt) {
			break;
		}
		printf("in copy %d \n", i);
		memcpy(&sack_p[i], iter_sack, sizeof(SACK_PACKET));
		_list_add(&(&sack_p[i++])->head, &packet->sack_head);
	}
	printf("end copy \n");
	_list_add(&packet->head, &resp_head.head);
	return packet;
}

static inline void resp_queue_delete(RESP_HEADER* packet) {
	list_del(&packet->head);
	SACK_PACKET* sack_p = NULL, *sack_tmp_p, *first_elm = NULL;
	first_elm =
			list_empty(&packet->sack_head) ?
					NULL :
					list_entry(packet->sack_head.prev, SACK_PACKET, head);
	list_for_each_prev_entry_safe(sack_p, sack_tmp_p, &packet->sack_head, head)
	{
		list_del(&sack_p->head);
	}
	if (!first_elm) {
		free(first_elm);
	}
	free(packet);
}

#endif /* RAILGUN_RESP_QUEUE_H_ */
