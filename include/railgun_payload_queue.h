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

#define for_packet_in_payload_queue_safe(s, n) list_for_each_prev_entry_safe(s, n, &payload_head.head, head)

#define for_packet_in_payload_queue(s) list_for_each_prev_entry(s, &payload_head.head, head)

#define is_payload_queue_empty list_empty(&(payload_head.head))

static inline RAILGUN_HEADER* payload_queue_begin() {
	return list_entry(payload_head.head.prev, RAILGUN_HEADER, head);
}

static inline RAILGUN_HEADER* payload_queue_allocate(int seq, int ack, int win,
		int src, void* src_param, u_int64_t time, int sack_size,
		SACK_PACKET* psack_queue_head) {
	int i = 0;
	SACK_PACKET* sack_p;
	RAILGUN_HEADER* packet = (RAILGUN_HEADER*) malloc(sizeof(RAILGUN_HEADER));
	bzero(packet, sizeof(RAILGUN_HEADER));
	packet->timestamp = time;
	packet->seq = seq;
	packet->ack = ack;
	packet->win = win;
	packet->src = 0;
	packet->sack_cnt =
			sack_size > MAX_DELIVER_SACK ? MAX_DELIVER_SACK : sack_size;
	INIT_LIST_HEAD(&packet->sack_head);
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
	if (src == 0) {
		packet->railgun_data_offset = (u_int32_t) src_param;
		packet->railgun_data_length = MTU
				- (4 + packet->sack_cnt * 2) * sizeof(u_int32_t);
	} else if (src == 1) {
		packet->railgun_data = src_param;
		packet->railgun_data_length = MTU
				- (4 + packet->sack_cnt * 2) * sizeof(u_int32_t);
	} else {
		packet->railgun_data_length = 0;
	}

	return packet;
}

static inline RAILGUN_HEADER* payload_queue_add_allocate(int seq, int ack,
		int win, u_int32_t offset, u_int64_t time, int sack_size,
		SACK_PACKET* psack_queue_head) {
	RAILGUN_HEADER* packet = payload_queue_allocate(seq, ack, win, 0,
			(void*) offset, time, sack_size, psack_queue_head);
	_list_add(&packet->head, &payload_head.head);
	return packet;
}

static inline RAILGUN_HEADER* zero_probe_allocate(int seq, int ack, int win,
		u_int64_t time, int sack_size, SACK_PACKET* psack_queue_head,
		int payload_size, void *probe_payload) {
	int i = 0;
	SACK_PACKET* sack_p;
	RAILGUN_HEADER* packet = (RAILGUN_HEADER*) malloc(sizeof(RAILGUN_HEADER));
	bzero(packet, sizeof(RAILGUN_HEADER));
	packet->timestamp = time;
	packet->seq = seq;
	packet->ack = ack;
	packet->win = win;
	packet->src = 1;
	packet->railgun_data_length = payload_size;
	packet->railgun_data = malloc(payload_size);
	memcpy(packet->railgun_data, probe_payload, payload_size);
	int max_sack_size = (MTU - 4 * 4) / 8;
	packet->sack_cnt = sack_size > max_sack_size ? max_sack_size : sack_size;
	INIT_LIST_HEAD(&packet->sack_head);
	if (packet->sack_cnt != 0) {
		sack_p = (SACK_PACKET*) calloc(packet->sack_cnt, sizeof(SACK_PACKET));
	}
	SACK_PACKET *iter_sack = NULL;
	list_for_each_prev_entry(iter_sack, &psack_queue_head->head, head)
	{
		if (i >= packet->sack_cnt) {
			break;
		}
		memcpy(&sack_p[i], iter_sack, sizeof(SACK_PACKET));
		_list_add(&(&sack_p[i++])->head, &packet->sack_head);
	}
	return packet;
}

static inline RAILGUN_HEADER* payload_queue_find(int seq) {
	RAILGUN_HEADER *p = NULL, *tmp = NULL;
	BOOL is_found = FALSE;
	for_packet_in_payload_queue_safe(p, tmp)
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

static inline void railgun_header_release(RAILGUN_HEADER* packet) {
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
	if (packet->src == 1) {
		free(packet->railgun_data);
	}
	free(packet);
}

static inline void payload_queue_delete(RAILGUN_HEADER* packet) {
	list_del(&packet->head);
	railgun_header_release(packet);
}

static inline void payload_queue_move_tail(RAILGUN_HEADER* packet) {
	list_move(&packet->head, &payload_head.head);
}

#endif /*RAILGUN_PAYLOAD_QUEUE_H_*/
