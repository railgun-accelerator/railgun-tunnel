/*
 * railgun_output.c
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */

#include <railgun_utils.h>

int railgun_packet_write(RAILGUN_PACKET* packet, int fd) {
	PAYLOAD_PACKET payload;
	memcpy(&payload, railgun_payload(packet), sizeof(PAYLOAD_PACKET));
	payload_htonl(&payload);
	int nbytes = write(fd, &payload, sizeof(PAYLOAD_PACKET));
	if (nbytes < 0) {
		perror("write error");
	}
	printf("send %d to server, seq = %d  \n", nbytes, packet->seq);
	return nbytes;
}

