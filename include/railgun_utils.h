/*
 * railgun_utils.h
 *
 *  Created on: 2014年12月2日
 *      Author: mabin
 */
#ifndef RAILGUN_UTILS_H_
#define RAILGUN_UTILS_H_

#include <railgun_common.h>

extern int set_non_blocking(int sockfd);

static inline void payload_htonl(PAYLOAD_PACKET* ppayload) {
	ppayload->seq = htonl(ppayload->seq);
	ppayload->ack = htonl(ppayload->ack);
}

static inline void payload_ntohl(PAYLOAD_PACKET* ppayload) {
	ppayload->seq = ntohl(ppayload->seq);
	ppayload->ack = ntohl(ppayload->ack);
}

static inline int before(u_int32_t seq1, u_int32_t seq2) {
	return (int32_t) (seq1 - seq2);
}

#define after(seq2, seq1) before(seq1, seq2);

static inline PAYLOAD_PACKET* railgun_payload(RAILGUN_PACKET* rp) {
	return (PAYLOAD_PACKET*) &rp->seq;
}

static inline u_int64_t get_current_time_in_millis() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#endif /*RAILGUN_UTILS_H_*/
