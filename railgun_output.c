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
	memcpy(&header, railgun_payload(packet), sizeof(PAYLOAD_HEADER));
	payload_htonl(&header);
	bzero(buffer, MAXBUF);
	memcpy(tmp_buf, &header, sizeof(u_int32_t));
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
		memcpy(tmp_buf, packet->data, nbytes);
	}
	nbytes = 0;
	while (nbytes < MTU) {
		int write_cnt = write(fd, &buffer, MTU);
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

