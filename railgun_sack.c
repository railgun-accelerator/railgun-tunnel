/*
 * railgun_sack.c
 *
 *  Created on: 2014年12月5日
 *      Author: mabin
 */

#include <railgun_sack.h>

void sack_queue_combine(int left, int right) {
	if (is_sack_queue_empty) {
		printf("is_sack_queue_empty, add \n");
		sack_queue_add(left, right);
	} else {
		SACK_PACKET* psack_p, *sack_tmp;
		for_sack_in_queue_safe(psack_p, sack_tmp)
		{
			if (left == psack_p->left_edge && right == psack_p->right_edge) {
				break;
			}
			if (left == psack_p->right_edge) {
				printf("left == psack_p->right_edge \n");
				printf(
						"left = %d, psack_p->left_edge = %d, psack_p->right_edge = %d \n",
						left, psack_p->left_edge, psack_p->right_edge);
				psack_p->right_edge = right;
				break;
			} else if (right == psack_p->left_edge) {
				printf("right == psack_p->left_edge \n");
				printf(
						"right = %d, psack_p->left_edge = %d, psack_p->right_edge = %d \n",
						right, psack_p->left_edge, psack_p->right_edge);
				psack_p->left_edge = left;
				break;
			} else if (left > psack_p->right_edge) {
				if (psack_p->head.prev == &sack_head.head) {
					sack_queue_add(left, right);
					break;
				}
			} else if (right < psack_p->left_edge) {
				printf("right < psack_p->left_edge \n");
				printf(
						"right = %d, psack_p->left_edge = %d, psack_p->right_edge = %d \n",
						right, psack_p->left_edge, psack_p->right_edge);
				sack_queue_add_tail(left, right);
				break;
			} else {
				continue;
			}
		}
	}
}
