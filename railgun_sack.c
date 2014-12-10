/*
 * railgun_sack.c
 *
 *  Created on: 2014年12月5日
 *      Author: mabin
 */

#include <railgun_sack.h>

void sack_queue_combine(SACK_PACKET* psack_new) {
	SACK_PACKET *psack, *tmp;
	if (is_sack_queue_empty) {
		_list_add(&psack_new->head, &sack_head.head);
		return;
	}
	list_for_each_prev_entry_safe(psack, tmp, &sack_head.head, head) {
		if (psack_new->left_edge == psack->left_edge && psack_new->right_edge == psack->right_edge) {
			free(psack_new);
			break;
		}
		if (psack_new->left_edge == psack->right_edge) {
			if (psack->head.prev != &sack_head.head) {
				psack = list_entry(psack->head.prev, SACK_PACKET, head);
				if (psack_new->right_edge == psack->left_edge) {
					list_entry(psack->head.next, SACK_PACKET, head)->right_edge = psack->right_edge;
					sack_queue_delete(psack);
					break;
				} else {
					list_entry(psack->head.next, SACK_PACKET, head)->right_edge = psack_new->right_edge;
					free(psack_new);
					break;
				}
			} else {
				psack->right_edge = psack_new->right_edge;
				free(psack_new);
				break;
			}
		} else if (psack_new->left_edge < psack->left_edge) {
			if (psack_new->right_edge == psack->left_edge) {
				psack->left_edge = psack_new->left_edge;
				free(psack_new);
				break;
			} else if (psack_new->right_edge < psack->left_edge) {
				list_add_tail(&psack_new->head, &psack->head);
				break;
			}
		} else if (psack_new->right_edge > psack->right_edge) {
			continue;
		} else {
			printf("try to insert a wield sack node");
			return;
		}
	}
}
