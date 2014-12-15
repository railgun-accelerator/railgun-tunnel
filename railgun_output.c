/*
 * railgun_output.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */

#include <railgun_utils.h>

int railgun_udp_write(RAILGUN_HEADER* packet, int fd, u_int8_t *buffer,
		u_int8_t *payload) {
	PAYLOAD_HEADER header;
	u_int8_t* tmp_buf = buffer;
	int nbytes = 0;
	memcpy(&header, railgun_payload_header(packet), sizeof(PAYLOAD_HEADER));
	payload_htonl(&header);
	bzero(buffer, MAXBUF);
	memcpy(tmp_buf, &header, 4 * sizeof(u_int32_t));
	tmp_buf += 4 * sizeof(u_int32_t);
	if (packet->sack_cnt != 0) {
		SACK_PACKET* psack_p = NULL;
		list_for_each_prev_entry(psack_p, &header.sack_head, head)
		{
			memcpy(tmp_buf, psack_p, 2 * sizeof(u_int32_t));
			tmp_buf += 2 * sizeof(u_int32_t);
		}
	}
	//if src represent is 0, just use payload as data source, otherwise use data
	if (packet->src == 0) {
		memcpy(tmp_buf, payload, packet->railgun_data_length);
	} else {
		memcpy(tmp_buf, packet->railgun_data, packet->railgun_data_length);
	}
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
	printf("send out %d , seq = %d  \n", nbytes, packet->seq);
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
		SACK_PACKET* psack_p = NULL;
		list_for_each_prev_entry(psack_p, &pheader->sack_head, head)
		{
			memcpy(tmp_buf, psack_p, 2 * sizeof(u_int32_t));
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
	printf("send %d to %s:%d, ack = %d  \n", write_cnt,
			inet_ntoa(resp->addr.sin_addr), ntohs(resp->addr.sin_port),
			ntohl(resp->ack));
}

