/*
 * railgun_sack.h
 *
 *  Created on: 2014年12月9日
 *      Author: mabin
 */

#ifndef RAILGUN_SACK_H_
#define RAILGUN_SACK_H_

#include <railgun_common.h>

extern SACK_PACKET sack_head;

#define for_sack_in_queue_safe(s, n) list_for_each_prev_entry_safe(s, n, &sack_head.head, head)

#define is_sack_queue_empty list_empty(&sack_head.head)

/**
 * sack combine strategy.
 */
void sack_queue_combine(int left, int right);

static inline int sack_queue_size() {
	int i = 0;
	struct list_head* phead;
	list_for_each_prev(phead, &sack_head.head)
	{
		++i;
	}
	return i;
}

static inline void sack_queue_add(int left, int right) {
	SACK_PACKET* packet = (SACK_PACKET*) malloc(sizeof(SACK_PACKET));
	packet->left_edge = left;
	packet->right_edge = right;
	_list_add(&packet->head, &sack_head.head);
}

static inline void sack_queue_add_tail(int left, int right) {
	SACK_PACKET* packet = (SACK_PACKET*) malloc(sizeof(SACK_PACKET));
	packet->left_edge = left;
	packet->right_edge = right;
	list_add_tail(&packet->head, &sack_head.head);
}

static inline SACK_PACKET* sack_queue_begin() {
	return list_entry(sack_head.head.prev, SACK_PACKET, head);
}

static inline void sack_queue_delete(SACK_PACKET* packet) {
	list_del(&packet->head);
	free(packet);
}

#endif /* RAILGUN_SACK_H_ */
