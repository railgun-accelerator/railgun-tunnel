/*
 * railgun_output.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */

#include <railgun_utils.h>

int railgun_packet_write(RAILGUN_HEADER* packet, int fd, u_int8_t *buffer,
		u_int8_t *payload, int *plength) {
	PAYLOAD_HEADER header;
	u_int8_t* tmp_buf = buffer;
	int nbytes = MTU - 3 * sizeof(u_int32_t);
	memcpy(&header, railgun_payload_header(packet), sizeof(PAYLOAD_HEADER));
	payload_htonl(&header);
	bzero(buffer, MAXBUF);
	memcpy(tmp_buf, &header, 3 * sizeof(u_int32_t));
	tmp_buf += 3 * sizeof(u_int32_t);
	if (packet->sack_cnt != 0) {
		int i = 0;
		for (; i < packet->sack_cnt; i++) {
			memcpy(tmp_buf, list_entry(&header.sack_head, SACK_PACKET, head),
					2 * sizeof(u_int32_t));
			nbytes -= 2 * sizeof(u_int32_t);
			tmp_buf += 2 * sizeof(u_int32_t);
		}
	}
	if (plength != NULL) {
		*plength = nbytes;
	}
	//if src represent is 0, just use payload as data source, otherwise use data
	if (packet->src == 0) {
		memcpy(tmp_buf, payload, nbytes);
	} else {
		memcpy(tmp_buf, packet->railgun_data, nbytes);
	}
	nbytes = 0;
	while (nbytes < MTU) {
		int write_cnt = write(fd, buffer, MTU);
		if (write_cnt < 0) {
			if (errno != EAGAIN) {
				perror("write error");
			}
			break;
		}
		nbytes += write_cnt;
	}
	printf("send %d to server, seq = %d  \n", nbytes, packet->seq);
	return nbytes;
}

void railgun_resp_send(RESP_HEADER* resp, int fd, u_int8_t *buffer) {
	PAYLOAD_HEADER* pheader = railgun_resp_header(resp);
	u_int8_t* tmp_buf = buffer;

	payload_htonl(pheader);
	bzero(buffer, MAXBUF);

	memcpy(tmp_buf, pheader, 3 * sizeof(u_int32_t));
	tmp_buf += 3 * sizeof(u_int32_t);

	int sack_cnt = ntohl(pheader->sack_cnt);

	if (sack_cnt != 0) {
		int i = 0;
		for (; i < sack_cnt; i++) {
			memcpy(tmp_buf, list_entry(&pheader->sack_head, SACK_PACKET, head),
					2 * sizeof(u_int32_t));
			tmp_buf += 2 * sizeof(u_int32_t);
		}
	}

	int write_cnt = sendto(fd, buffer, tmp_buf - buffer, 0,
			(struct sockaddr *) &resp->addr, sizeof(resp->addr));
	if (write_cnt < 0) {
		if (errno != EAGAIN) {
			perror("write error");
		}
	}
	printf("send %d to %s:%d, ack = %d  \n", write_cnt, inet_ntoa(resp->addr.sin_addr),
			ntohs(resp->addr.sin_port), ntohl(resp->ack));
}

