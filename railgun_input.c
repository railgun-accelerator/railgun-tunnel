/*
 * railgun_input.c
 *
 *  Created on: 2014年12月5日
 *      Author: mabin
 */

#include <railgun_utils.h>

int railgun_packet_read(int fd, u_int8_t *buffer, RESP_HEADER *pheader) {
	int nbytes = 0, sack_cnt = 0;
	while (nbytes < MTU) {
		int read_cnt = recvfrom(listen_fd, &buffer, MTU, 0,
				(struct sockaddr*) &pheader->addr, &pheader->addr_len);
		if (read_cnt < 0) {
			if (errno != EAGAIN) {
				perror("read error");
			}
		}
		nbytes += read_cnt;
	}
	int pos = 0, i;
	pheader->ack = ntohl(*(u_int32_t*)buffer[pos]);
	pos += 4;
	pheader->seq = ntohl(*(u_int32_t*)buffer[pos]);
	pos += 4;
	pheader->sack_cnt = sack_cnt = ntohl(*(u_int32_t*)buffer[pos]);
	pos += 4;
	INIT_LIST_HEAD(&pheader->sack_head);
	if (sack_cnt != 0) {
		for (i = 0; i < pheader->sack_cnt; i++) {
			SACK_PACKET* sp = (SACK_PACKET*)malloc(sizeof(SACK_PACKET*));
			sp->left_edge = ntohl(*(u_int32_t*)buffer[pos]);
			pos += 4;
			sp->right_edge = ntohl(*(u_int32_t*)buffer[pos]);
			_list_add()
		}
	} else {

	}
}

