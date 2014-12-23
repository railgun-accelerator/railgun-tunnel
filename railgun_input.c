/*
 * railgun_input.c
 *
 *  Created on: 2014年12月5日
 *      Author: mabin
 */

#include <railgun_utils.h>

int railgun_udp_read(int fd, u_int8_t *buffer, RAILGUN_HEADER *pheader, int *payload_pos) {
	int nbytes = 0, pos = 0, sack_cnt = 0, i;
	while (nbytes <= MTU) {
		int read_cnt = recvfrom(fd, buffer, MTU, 0,
				(struct sockaddr*) &pheader->addr, &pheader->addr_len);
		if (read_cnt < 0) {
			if (errno != EAGAIN) {
				perror("read error");
			}
			break;
		}
		nbytes += read_cnt;
	}
	pheader->seq = ntohl(*(u_int32_t*) &buffer[pos]);
	pos += 4;
	pheader->ack = ntohl(*(u_int32_t*) &buffer[pos]);
	pos += 4;
	pheader->win = ntohl(*(u_int32_t*) &buffer[pos]);
	pos += 4;
	pheader->sack_cnt = sack_cnt = ntohl(*(u_int32_t*) &buffer[pos]);
	pos += 4;
	INIT_LIST_HEAD(&pheader->sack_head);
	if (sack_cnt != 0) {
		for (i = 0; i < pheader->sack_cnt; i++) {
			SACK_PACKET* sp = (SACK_PACKET*) malloc(sizeof(SACK_PACKET));
			sp->left_edge = ntohl(*(u_int32_t*) &buffer[pos]);
			pos += 4;
			sp->right_edge = ntohl(*(u_int32_t*) &buffer[pos]);
			pos += 4;
			_list_add(&sp->head, &pheader->sack_head);
		}
	}
	*payload_pos = pos;
	return nbytes;
}

int railgun_tcp_read(int fd, u_int8_t *buffer, int start_pos,
		int *pmax_avail_size) {
	int nbytes = 0, read_cnt = 0;
	while (nbytes <= *pmax_avail_size) {
		read_cnt = read(fd, buffer + start_pos + nbytes, *pmax_avail_size);
		if (read_cnt < 0) {
			if (errno != EAGAIN) {
				perror("read error");
			}
			break;
		}
		nbytes += read_cnt;
	}
	*pmax_avail_size = nbytes;
	if (read_cnt < 0) {
		nbytes = read_cnt;
	}
	return nbytes;
}

